#include "Downloads.h"

#include "DownloadPlanner.h"
#include "PlayLog.h"
#include "LibraryActions.h"
#include "TrackStorage.h"
#include "core/Logging.h"
#include "core/Paths.h"
#include "innertube/Session.h"
#include "media/PlaybackController.h"
#include "net/Connectivity.h"
#include "net/HttpClient.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSettings>

#include <algorithm>

namespace {

constexpr qint64 kGigabyte = 1024LL * 1024 * 1024;
constexpr qint64 kRetrySeconds = 3600;

QString extensionFor(const QString &mimeType)
{
    return mimeType.contains(QLatin1String("mp4")) ? QStringLiteral("m4a") : QStringLiteral("weba");
}

bool safeId(const QString &videoId)
{
    static const QRegularExpression pattern(QStringLiteral("^[a-zA-Z0-9_-]{11}$"));
    return pattern.match(videoId).hasMatch();
}

}

namespace library {

Downloads::Downloads(QObject *parent)
    : QObject(parent)
    , m_artworkCache(this)
    , m_resolver(innertube::Session::instance())
{
    m_resolver.setBackground([] { return !media::PlaybackController::instance().resolving(); });
    const QSettings settings;
    m_smartEnabled = settings.value(QStringLiteral("downloads/enabled"), true).toBool();
    m_trackLimit = qBound(1, settings.value(QStringLiteral("downloads/tracks"), 200).toInt(), 2000);
    m_sizeLimitGb =
        qBound(0.1, settings.value(QStringLiteral("downloads/gigabytes"), 2).toDouble(), 100.0);
    connect(&m_resolver, &player::StreamResolver::resolved, this, &Downloads::fetch);
    connect(&m_resolver, &player::StreamResolver::failed, this,
            [this](const QString &videoId, const QString &message, bool unreachable) {
        if (videoId != m_activeVideoId)
            return;
        if (unreachable)
            defer(videoId);
        else
            fail(videoId, message);
    });
    connect(&net::Connectivity::instance(), &net::Connectivity::onlineChanged, this, [this] {
        if (!net::Connectivity::instance().online())
            return;
        restoreArtwork();
        pump();
    });
    m_pumpTimer.setInterval(1000);
    connect(&m_pumpTimer, &QTimer::timeout, this, [this] {
        removeReleased();
        pump();
    });
    m_pumpTimer.start();
    m_replanTimer.setSingleShot(true);
    m_replanTimer.setInterval(350);
    connect(&m_replanTimer, &QTimer::timeout, this, &Downloads::cleanUp);
    m_plannerTimer.setInterval(15 * 60 * 1000);
    connect(&m_plannerTimer, &QTimer::timeout, this, &Downloads::cleanUp);
    m_plannerTimer.start();
    load();
    connect(&LibraryActions::instance(), &LibraryActions::trackRatingChanged, this,
            [this](const QString &id, int rating) {
        const auto entry = m_entries.find(id);
        if (entry == m_entries.end())
            return;
        entry->track.liked = rating > 0;
        save();
    });
    QTimer::singleShot(30000, this, &Downloads::cleanUp);
}

Downloads &Downloads::instance()
{
    static auto *downloads = new Downloads(QCoreApplication::instance());
    return *downloads;
}

Downloads *Downloads::create(QQmlEngine *, QJSEngine *)
{
    QQmlEngine::setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
    return &instance();
}

QString Downloads::directory() const
{
    const QString path = core::paths::dataDir() + QStringLiteral("/downloads");
    QDir().mkpath(path);
    return path;
}

QString Downloads::indexFile() const
{
    return directory() + QStringLiteral("/downloads.json");
}

QString Downloads::partialFile(const QString &videoId) const
{
    return directory() + QLatin1Char('/') + videoId + QStringLiteral(".part");
}

qint64 Downloads::ceiling() const
{
    return qint64(m_sizeLimitGb * kGigabyte);
}

void Downloads::load()
{
    QFile file(indexFile());
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    const QJsonObject root = document.object();
    if (!document.isArray() && root.value(QStringLiteral("version")).toInt() != 1)
        return;
    const QJsonArray stored =
        document.isArray() ? document.array() : root.value(QStringLiteral("entries")).toArray();
    for (const QJsonValue &value : stored) {
        const QJsonObject object = value.toObject();
        Entry entry;
        entry.track = trackStorage::decode(object);
        if (!safeId(entry.track.videoId))
            continue;
        const QString name = QFileInfo(object.value(QStringLiteral("file")).toString()).fileName();
        if (name != entry.track.videoId + QStringLiteral(".m4a")
            && name != entry.track.videoId + QStringLiteral(".weba")
            && name != entry.track.videoId + QStringLiteral(".audio"))
            continue;
        entry.file = directory() + QLatin1Char('/') + name;
        const QFileInfo info(entry.file);
        if (!info.isFile() || info.size() <= 0)
            continue;
        entry.bytes = info.size();
        entry.itag = object.value(QStringLiteral("itag")).toInt();
        entry.fetchedAt = qint64(object.value(QStringLiteral("fetchedAt")).toDouble());
        entry.forced = object.value(QStringLiteral("forced")).toBool(true);
        entry.gainDb = object.value(QStringLiteral("gainDb")).toDouble();
        entry.hasGain = object.contains(QStringLiteral("gainDb"));
        m_entries.insert(entry.track.videoId, entry);
        m_states.insert(entry.track.videoId, Ready);
    }
    const QJsonObject partials = root.value(QStringLiteral("partials")).toObject();
    for (auto it = partials.begin(); it != partials.end(); ++it) {
        if (safeId(it.key()))
            m_partialItags.insert(it.key(), it.value().toInt());
    }
    for (const QJsonValue &value : root.value(QStringLiteral("excluded")).toArray())
        m_excluded.insert(value.toString());
    for (const QJsonValue &value : root.value(QStringLiteral("removals")).toArray())
        m_removals.insert(value.toString());
    for (const QJsonValue &value : root.value(QStringLiteral("pending")).toArray()) {
        const QJsonObject object = value.toObject();
        const media::Track track = trackStorage::decode(object);
        if (!safeId(track.videoId) || m_entries.contains(track.videoId))
            continue;
        const bool forced = object.value(QStringLiteral("forced")).toBool();
        if (!forced && !m_smartEnabled)
            continue;
        m_wanted.insert(track.videoId, track);
        m_partialItags.insert(track.videoId, object.value(QStringLiteral("itag")).toInt());
        if (forced)
            m_forced.insert(track.videoId);
        m_queue.append(track.videoId);
        m_states.insert(track.videoId, Waiting);
    }
    save();
}

bool Downloads::save() const
{
    QJsonArray stored;
    for (const Entry &entry : m_entries) {
        QJsonObject object = trackStorage::encode(entry.track);
        object.insert(QStringLiteral("file"), QFileInfo(entry.file).fileName());
        object.insert(QStringLiteral("bytes"), double(entry.bytes));
        object.insert(QStringLiteral("itag"), entry.itag);
        object.insert(QStringLiteral("fetchedAt"), double(entry.fetchedAt));
        object.insert(QStringLiteral("forced"), entry.forced);
        if (entry.hasGain)
            object.insert(QStringLiteral("gainDb"), entry.gainDb);
        stored.append(object);
    }
    QJsonArray pending;
    for (const media::Track &track : m_wanted) {
        QJsonObject object = trackStorage::encode(track);
        object.insert(QStringLiteral("forced"), m_forced.contains(track.videoId));
        object.insert(QStringLiteral("itag"), m_partialItags.value(track.videoId));
        pending.append(object);
    }
    QJsonObject partials;
    for (auto it = m_partialItags.cbegin(); it != m_partialItags.cend(); ++it)
        partials.insert(it.key(), it.value());
    const QJsonObject root {
        {QStringLiteral("partials"), partials},
        {QStringLiteral("version"), 1},
        {QStringLiteral("entries"), stored},
        {QStringLiteral("pending"), pending},
        {QStringLiteral("excluded"), QJsonArray::fromStringList(m_excluded.values())},
        {QStringLiteral("removals"), QJsonArray::fromStringList(m_removals.values())},
    };
    QSaveFile file(indexFile());
    if (!file.open(QIODevice::WriteOnly))
        return false;
    const QByteArray bytes = QJsonDocument(root).toJson(QJsonDocument::Compact);
    return file.write(bytes) == bytes.size() && file.commit();
}

QStringList Downloads::ids() const
{
    QStringList result;
    for (auto entry = m_entries.cbegin(); entry != m_entries.cend(); ++entry) {
        if (!m_removals.contains(entry.key()))
            result.append(entry.key());
    }
    return result;
}

QList<media::Track> Downloads::tracks() const
{
    QList<Entry> entries = m_entries.values();
    std::ranges::sort(entries, [](const Entry &left, const Entry &right) {
        return left.fetchedAt == right.fetchedAt ? left.track.videoId < right.track.videoId
                                                 : left.fetchedAt > right.fetchedAt;
    });
    QList<media::Track> tracks;
    for (const Entry &entry : entries) {
        if (!m_removals.contains(entry.track.videoId))
            tracks.append(entry.track);
    }
    return tracks;
}

bool Downloads::isForced(const QString &videoId) const
{
    const auto entry = m_entries.constFind(videoId);
    return entry != m_entries.constEnd() ? entry->forced : m_forced.contains(videoId);
}

int Downloads::stateOf(const QString &videoId) const
{
    return m_removals.contains(videoId) ? Absent : m_states.value(videoId, Absent);
}

double Downloads::partialBytes() const
{
    qint64 bytes = 0;
    for (const QString &id : m_partialItags.keys())
        bytes += qMax(qint64(0), QFileInfo(partialFile(id)).size());
    return double(bytes);
}

double Downloads::smartBytes() const
{
    qint64 bytes = 0;
    for (const Entry &entry : m_entries) {
        if (!entry.forced)
            bytes += entry.bytes;
    }
    return double(bytes);
}

double Downloads::forcedBytes() const
{
    qint64 bytes = 0;
    for (const Entry &entry : m_entries) {
        if (entry.forced)
            bytes += entry.bytes;
    }
    return double(bytes);
}

std::optional<player::Stream> Downloads::localStream(const media::Track &track) const
{
    const auto entry = m_entries.constFind(track.videoId);
    if (entry == m_entries.constEnd() || m_removals.contains(track.videoId)
        || !QFileInfo::exists(entry->file))
        return std::nullopt;
    player::Stream stream;
    stream.videoId = track.videoId;
    stream.url = QUrl::fromLocalFile(entry->file);
    stream.title = track.title.isEmpty() ? entry->track.title : track.title;
    stream.artist = track.artist.isEmpty() ? entry->track.artist : track.artist;
    stream.durationMs = track.durationMs > 0 ? track.durationMs : entry->track.durationMs;
    stream.itag = entry->itag;
    if (entry->hasGain)
        stream.gainDb = entry->gainDb;
    return stream;
}

void Downloads::saveSettings()
{
    QSettings settings;
    settings.setValue(QStringLiteral("downloads/enabled"), m_smartEnabled);
    settings.setValue(QStringLiteral("downloads/tracks"), m_trackLimit);
    settings.setValue(QStringLiteral("downloads/gigabytes"), m_sizeLimitGb);
    Q_EMIT settingsChanged();
    m_replanTimer.start();
}

void Downloads::setSmartEnabled(bool enabled)
{
    if (m_smartEnabled == enabled)
        return;
    m_smartEnabled = enabled;
    saveSettings();
}

void Downloads::setTrackLimit(int count)
{
    count = qBound(1, count, 2000);
    if (m_trackLimit == count)
        return;
    m_trackLimit = count;
    saveSettings();
}

void Downloads::setSizeLimitGb(double gigabytes)
{
    gigabytes = qBound(0.1, gigabytes, 100.0);
    if (qFuzzyCompare(m_sizeLimitGb, gigabytes))
        return;
    m_sizeLimitGb = gigabytes;
    saveSettings();
}

void Downloads::keep(const QList<media::Track> &tracks)
{
    enqueue(tracks, true);
}

int Downloads::smartPending() const
{
    int pending = 0;
    for (auto id = m_wanted.keyBegin(); id != m_wanted.keyEnd(); ++id) {
        if (!m_forced.contains(*id))
            ++pending;
    }
    return pending;
}

double Downloads::progress() const
{
    if (m_batchTotal <= 1)
        return m_progress;
    return qBound(0.0, (m_batchDone + m_progress) / m_batchTotal, 1.0);
}

void Downloads::openBatch(int added)
{
    if (added <= 0)
        return;
    if (m_batchDone >= m_batchTotal)
        m_batchTotal = m_batchDone = 0;
    m_batchTotal += added;
}

void Downloads::closeBatchItem()
{
    if (m_batchTotal == 0)
        return;
    if (++m_batchDone >= m_batchTotal)
        m_batchTotal = m_batchDone = 0;
}

void Downloads::dropFromBatch(const QString &videoId)
{
    if (m_batchTotal == 0 || !m_forced.contains(videoId) || !m_wanted.contains(videoId))
        return;
    if (--m_batchTotal <= m_batchDone)
        m_batchTotal = m_batchDone = 0;
}

void Downloads::browseFiles() const
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(directory()));
}

void Downloads::enqueue(const QList<media::Track> &tracks, bool forced)
{
    int added = 0;
    int retained = 0;
    int queued = 0;
    for (const media::Track &track : tracks) {
        if (!safeId(track.videoId) || track.live)
            continue;
        if (forced) {
            m_excluded.remove(track.videoId);
            m_removals.remove(track.videoId);
        }
        const auto stored = m_entries.find(track.videoId);
        if (stored != m_entries.end() && QFileInfo::exists(stored->file)) {
            if (forced) {
                stored->forced = true;
                m_states.insert(track.videoId, Ready);
                ++retained;
            }
            continue;
        }
        if (stored != m_entries.end())
            m_entries.erase(stored);
        const bool promoted = forced && !m_forced.contains(track.videoId);
        if (forced)
            m_forced.insert(track.videoId);
        if (m_wanted.contains(track.videoId)) {
            if (promoted)
                ++added;
            ++queued;
            continue;
        }
        m_wanted.insert(track.videoId, track);
        m_queue.append(track.videoId);
        m_states.insert(track.videoId, Waiting);
        ++added;
    }
    if (forced)
        openBatch(added);
    save();
    Q_EMIT changed();
    if (retained > 0)
        Q_EMIT catalogueChanged();
    Q_EMIT progressChanged();
    if (forced) {
        if (added == 0 && queued == 0 && retained == 0)
            Q_EMIT feedback(tr("No tracks available to download."));
        else if (added == 0 && queued > 0)
            Q_EMIT feedback(tr("Download already queued."));
        else if (added == 0 && retained > 0)
            Q_EMIT feedback(tr("Download kept on this device."));
        else if (!net::Connectivity::instance().online())
            Q_EMIT feedback(tr("Download queued. It starts when you're back online."));
        else if (!m_activeVideoId.isEmpty() || media::PlaybackController::instance().resolving())
            Q_EMIT feedback(tr("Download queued."));
    }
    pump();
}

void Downloads::cancel(const QString &videoId)
{
    dropFromBatch(videoId);
    m_wanted.remove(videoId);
    m_queue.removeAll(videoId);
    m_forced.remove(videoId);
    m_states.remove(videoId);
    if (m_activeVideoId == videoId) {
        m_activeVideoId.clear();
        if (m_transfer)
            m_transfer->cancel();
        m_transfer.clear();
    }
    m_partialItags.remove(videoId);
    Q_EMIT progressChanged();
    if (safeId(videoId))
        QFile::remove(partialFile(videoId));
}

void Downloads::discard(const QList<media::Track> &tracks)
{
    for (const media::Track &track : tracks) {
        cancel(track.videoId);
        m_excluded.insert(track.videoId);
        if (m_entries.contains(track.videoId))
            m_removals.insert(track.videoId);
    }
    removeReleased();
    save();
    Q_EMIT feedback(tr("Download removed"));
    Q_EMIT changed();
    Q_EMIT catalogueChanged();
    pump();
}

void Downloads::removeReleased()
{
    bool touched = false;
    for (const QString &id : m_removals.values()) {
        if (media::PlaybackController::instance().usesDownload(id))
            continue;
        const auto entry = m_entries.constFind(id);
        if (entry != m_entries.constEnd()) {
            if (QFileInfo::exists(entry->file) && !QFile::remove(entry->file))
                continue;
            QFile::remove(directory() + QLatin1Char('/') + id + QStringLiteral(".jpg"));
            m_entries.erase(entry);
            m_states.remove(id);
            touched = true;
        }
        m_removals.remove(id);
    }
    if (touched) {
        save();
        Q_EMIT changed();
        Q_EMIT catalogueChanged();
    }
}

void Downloads::removeAll()
{
    m_smartEnabled = false;
    for (const media::Track &track : m_wanted.values())
        cancel(track.videoId);
    for (const QString &id : m_partialItags.keys())
        cancel(id);
    discard(tracks());
    saveSettings();
}

void Downloads::cleanUp()
{
    m_replanTimer.stop();
    for (const QString &id : m_partialItags.keys()) {
        const QFileInfo file(partialFile(id));
        if (m_wanted.contains(id))
            continue;
        if (!file.exists() || file.lastModified().secsTo(QDateTime::currentDateTime()) > 86400) {
            QFile::remove(file.absoluteFilePath());
            m_partialItags.remove(id);
        }
    }
    Q_EMIT progressChanged();
    QHash<QString, qint64> sizes;
    QSet<QString> forced = m_forced;
    for (auto entry = m_entries.cbegin(); entry != m_entries.cend(); ++entry) {
        if (!QFileInfo::exists(entry->file))
            m_removals.insert(entry.key());
        sizes.insert(entry.key(), entry->bytes);
        if (entry->forced)
            forced.insert(entry.key());
    }
    QList<media::Track> ranked = DownloadPlanner::rank(PlayLog::instance().entries());
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    ranked.removeIf([this, now](const media::Track &track) {
        return m_excluded.contains(track.videoId) || m_retryAfter.value(track.videoId) > now;
    });
    const QSet<QString> selected = m_smartEnabled
        ? DownloadPlanner::choose(ranked, m_trackLimit,
                                  qMax(qint64(0), ceiling() - qint64(forcedBytes())), sizes, forced)
        : QSet<QString>();
    for (auto entry = m_entries.cbegin(); entry != m_entries.cend(); ++entry) {
        if (!entry->forced && !selected.contains(entry.key()))
            m_removals.insert(entry.key());
        else if (QFileInfo::exists(entry->file) && !m_excluded.contains(entry.key()))
            m_removals.remove(entry.key());
    }
    for (const QString &id : m_wanted.keys()) {
        if (!m_forced.contains(id) && !selected.contains(id))
            cancel(id);
    }
    removeReleased();
    QList<media::Track> wanted;
    for (const media::Track &track : ranked) {
        if (selected.contains(track.videoId))
            wanted.append(track);
    }
    if (!wanted.isEmpty())
        enqueue(wanted, false);
    else {
        save();
        Q_EMIT changed();
    }
}

void Downloads::pump()
{
    if (!m_activeVideoId.isEmpty() || m_queue.isEmpty()
        || media::PlaybackController::instance().resolving()
        || !net::Connectivity::instance().settled())
        return;
    const auto manual =
        std::ranges::find_if(m_queue, [this](const QString &id) { return m_forced.contains(id); });
    const qsizetype next = manual == m_queue.end() ? 0 : manual - m_queue.begin();
    m_activeVideoId = m_queue.takeAt(next);
    const media::Track track = m_wanted.value(m_activeVideoId);
    if (m_forced.contains(track.videoId))
        reserveSpace(qMax(qint64(60), track.durationMs / 1000) * 24000);
    m_progress = 0;
    m_states.insert(m_activeVideoId, Fetching);
    Q_EMIT changed();
    Q_EMIT progressChanged();
    m_resolver.resolve(track.videoId, track.upload, true);
}

void Downloads::fetch(const player::Stream &stream)
{
    if (stream.videoId != m_activeVideoId || !m_wanted.contains(stream.videoId))
        return;
    if (stream.live) {
        refuseLive(stream.videoId);
        return;
    }
    if (media::PlaybackController::instance().resolving()) {
        QTimer::singleShot(500, this, [this, stream] { fetch(stream); });
        return;
    }
    const QString path = partialFile(stream.videoId);
    if (m_partialItags.value(stream.videoId) != stream.itag)
        QFile::remove(path);
    m_partialItags.insert(stream.videoId, stream.itag);
    if (!save()) {
        fail(stream.videoId, tr("Could not write the download index."));
        return;
    }
    const qint64 offset = qMax(qint64(0), QFileInfo(path).size());
    net::Headers headers {
        {QByteArrayLiteral("user-agent"), stream.userAgent.toUtf8()},
        {QByteArrayLiteral("range"),
         QByteArrayLiteral("bytes=") + QByteArray::number(offset) + '-'},
    };
    if (!stream.cookie.isEmpty())
        headers.append({QByteArrayLiteral("cookie"), stream.cookie.toUtf8()});
    const qint64 available =
        qMax(qint64(0), ceiling() - qint64(smartBytes() + forcedBytes() + partialBytes()) + offset);
    m_transfer = net::HttpClient::instance().download(stream.url, headers, path, offset, available);
    connect(m_transfer, &net::DownloadTransfer::progress, this,
            [this](qint64 received, qint64 total) {
        m_progress = total > 0 ? qBound(0.0, double(received) / total, 1.0) : 0;
        Q_EMIT progressChanged();
    });
    connect(m_transfer, &net::DownloadTransfer::finished, this,
            [this, stream, transfer = m_transfer](const QString &error) {
        if (transfer)
            transfer->deleteLater();
        if (stream.videoId != m_activeVideoId)
            return;
        m_transfer.clear();
        if (transfer && transfer->restartRequired()) {
            m_activeVideoId.clear();
            m_states.insert(stream.videoId, Waiting);
            m_queue.prepend(stream.videoId);
            Q_EMIT changed();
            pump();
        } else if (error.isEmpty())
            store(stream);
        else if (transfer && transfer->unreachable())
            defer(stream.videoId);
        else
            fail(stream.videoId, error);
    });
}

void Downloads::store(const player::Stream &stream)
{
    Entry entry;
    entry.track = m_wanted.value(stream.videoId);
    entry.file = directory() + QLatin1Char('/') + stream.videoId + QLatin1Char('.')
        + extensionFor(stream.mimeType);
    entry.hasGain = stream.gainDb.has_value();
    entry.gainDb = stream.gainDb.value_or(0.0);
    entry.itag = stream.itag;
    entry.fetchedAt = QDateTime::currentSecsSinceEpoch();
    entry.forced = m_forced.contains(stream.videoId);
    entry.bytes = QFileInfo(partialFile(stream.videoId)).size();
    if (!QFile::rename(partialFile(stream.videoId), entry.file)) {
        fail(stream.videoId, tr("Could not finish writing the download."));
        return;
    }
    if (entry.track.durationMs == 0)
        entry.track.durationMs = stream.durationMs;
    m_entries.insert(stream.videoId, entry);
    m_partialItags.remove(stream.videoId);
    m_wanted.remove(stream.videoId);
    if (!save()) {
        m_entries.remove(stream.videoId);
        QFile::remove(entry.file);
        fail(stream.videoId, tr("Could not save the download index."));
        return;
    }
    m_forced.remove(stream.videoId);
    m_activeVideoId.clear();
    m_states.insert(stream.videoId, Ready);
    if (entry.forced)
        closeBatchItem();
    if (m_batchTotal == 0 && entry.forced)
        Q_EMIT feedback(tr("Download complete"));
    Q_EMIT changed();
    Q_EMIT catalogueChanged();
    Q_EMIT progressChanged();
    storeArtwork(stream.videoId);
    pump();
}

void Downloads::reserveSpace(qint64 bytes)
{
    QList<Entry> removable;
    const QList<media::Track> ranked = DownloadPlanner::rank(PlayLog::instance().entries());
    QHash<QString, int> priorities;
    for (int i = 0; i < ranked.size(); ++i)
        priorities.insert(ranked.at(i).videoId, i);
    for (const Entry &entry : m_entries) {
        if (!entry.forced)
            removable.append(entry);
    }
    std::ranges::sort(removable, [&priorities](const Entry &left, const Entry &right) {
        return priorities.value(left.track.videoId, 200000)
            > priorities.value(right.track.videoId, 200000);
    });
    qint64 used = qint64(smartBytes() + forcedBytes());
    for (const Entry &entry : removable) {
        if (used + bytes <= ceiling())
            break;
        if (media::PlaybackController::instance().usesDownload(entry.track.videoId))
            continue;
        m_removals.insert(entry.track.videoId);
        used -= entry.bytes;
    }
    removeReleased();
}

void Downloads::storeArtwork(const QString &videoId)
{
    const QString source = m_entries.value(videoId).track.artId;
    if (source.isEmpty() || QUrl(source).isLocalFile())
        return;
    m_artworkCache.load(source, QSize(512, 512), [this, videoId](const QImage &image) {
        const auto entry = m_entries.find(videoId);
        if (entry == m_entries.end() || image.isNull())
            return;
        const QString path = directory() + QLatin1Char('/') + videoId + QStringLiteral(".jpg");
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly) || !image.save(&file, "JPG") || !file.commit())
            return;
        entry->track.artId = QUrl::fromLocalFile(path).toString();
        save();
        Q_EMIT catalogueChanged();
    });
}

void Downloads::restoreArtwork()
{
    QStringList missing;
    for (auto entry = m_entries.cbegin(); entry != m_entries.cend(); ++entry) {
        if (!entry->track.artId.isEmpty() && !QUrl(entry->track.artId).isLocalFile())
            missing.append(entry.key());
    }
    for (const QString &videoId : std::as_const(missing))
        storeArtwork(videoId);
}

void Downloads::defer(const QString &videoId)
{
    qCDebug(logStream) << "download waits for the network" << videoId;
    m_activeVideoId.clear();
    m_states.insert(videoId, Waiting);
    m_queue.prepend(videoId);
    Q_EMIT changed();
    Q_EMIT progressChanged();
}

void Downloads::refuseLive(const QString &videoId)
{
    qCDebug(logStream) << "live streams are not downloadable" << videoId;
    const bool forced = m_forced.remove(videoId);
    if (forced)
        closeBatchItem();
    m_wanted.remove(videoId);
    m_activeVideoId.clear();
    m_states.remove(videoId);
    m_excluded.insert(videoId);
    QFile::remove(partialFile(videoId));
    save();
    Q_EMIT progressChanged();
    if (forced)
        Q_EMIT feedback(tr("Live streams can't be downloaded."));
    Q_EMIT changed();
    pump();
}

void Downloads::fail(const QString &videoId, const QString &message)
{
    qCDebug(logStream) << "download failed" << videoId << message;
    const bool forced = m_forced.remove(videoId);
    if (forced)
        closeBatchItem();
    const QString title = m_wanted.value(videoId).title;
    m_wanted.remove(videoId);
    m_activeVideoId.clear();
    m_states.insert(videoId, Failed);
    m_retryAfter.insert(videoId, QDateTime::currentSecsSinceEpoch() + kRetrySeconds);
    save();
    Q_EMIT progressChanged();
    if (forced)
        Q_EMIT feedback(tr("Could not download that track. %1").arg(message));
    else
        Q_EMIT smartFailed(title);
    Q_EMIT changed();
    pump();
}

}
