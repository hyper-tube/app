#include "SessionStore.h"

#include "core/Json.h"
#include "core/Logging.h"
#include "core/Paths.h"
#include "media/PlaybackController.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QTimeZone>

namespace {

constexpr int kSaveDelayMs = 2000;
constexpr int kMaxStoredTracks = 400;
constexpr qint64 kTailMs = 5000;

QJsonObject encode(const media::Track &track)
{
    QJsonObject entry {
        {QStringLiteral("videoId"), track.videoId},
        {QStringLiteral("title"), track.title},
    };
    if (!track.artist.isEmpty())
        entry.insert(QStringLiteral("artist"), track.artist);
    if (!track.album.isEmpty())
        entry.insert(QStringLiteral("album"), track.album);
    if (!track.artId.isEmpty())
        entry.insert(QStringLiteral("artId"), track.artId);
    if (track.durationMs > 0)
        entry.insert(QStringLiteral("durationMs"), double(track.durationMs));
    if (track.video)
        entry.insert(QStringLiteral("video"), true);
    if (track.upload)
        entry.insert(QStringLiteral("upload"), true);
    if (track.live)
        entry.insert(QStringLiteral("live"), true);
    if (track.episode)
        entry.insert(QStringLiteral("episode"), true);
    return entry;
}

const QStringList kSourceFields {
    QStringLiteral("title"),  QStringLiteral("subtitle"), QStringLiteral("artId"),
    QStringLiteral("kind"),   QStringLiteral("browseId"), QStringLiteral("playlistId"),
    QStringLiteral("params"),
};

QJsonObject encodeSource(const model::Item &source)
{
    const QStringList values {source.title,    source.subtitle,   source.artId, source.kind,
                              source.browseId, source.playlistId, source.params};
    QJsonObject entry;
    for (qsizetype field = 0; field < kSourceFields.size(); ++field) {
        if (!values.at(field).isEmpty())
            entry.insert(kSourceFields.at(field), values.at(field));
    }
    return entry;
}

model::Item decodeSource(const QJsonObject &entry)
{
    model::Item source;
    const QList<QString *> targets {&source.title, &source.subtitle, &source.artId,
                                    &source.kind,  &source.browseId, &source.playlistId,
                                    &source.params};
    for (qsizetype field = 0; field < kSourceFields.size(); ++field)
        *targets.at(field) = entry.value(kSourceFields.at(field)).toString();
    return source.valid() ? source : model::Item();
}

media::Track decode(const QJsonObject &entry)
{
    media::Track track;
    track.videoId = entry.value(QStringLiteral("videoId")).toString();
    track.title = entry.value(QStringLiteral("title")).toString();
    track.artist = entry.value(QStringLiteral("artist")).toString();
    track.album = entry.value(QStringLiteral("album")).toString();
    track.artId = entry.value(QStringLiteral("artId")).toString();
    track.durationMs = core::json::toInt(entry.value(QStringLiteral("durationMs")));
    track.video = entry.value(QStringLiteral("video")).toBool();
    track.upload = entry.value(QStringLiteral("upload")).toBool();
    track.live = entry.value(QStringLiteral("live")).toBool();
    track.episode = entry.value(QStringLiteral("episode")).toBool();
    return track;
}

}

namespace platform {

SessionStore::SessionStore(media::PlaybackController &controller, QObject *parent)
    : QObject(parent)
    , m_controller(controller)
    , m_path(core::paths::dataDir() + QStringLiteral("/session.json"))
{
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(kSaveDelayMs);
    connect(&m_saveTimer, &QTimer::timeout, this, &SessionStore::save);

    using Controller = media::PlaybackController;
    const auto schedule = [this] { m_saveTimer.start(); };
    connect(&m_controller, &Controller::queueChanged, this, schedule);
    connect(&m_controller, &Controller::queueSourceChanged, this, schedule);
    connect(&m_controller, &Controller::queueIndexChanged, this, schedule);
    connect(&m_controller, &Controller::playingChanged, this, [this] {
        m_playedAt = QDateTime::currentDateTimeUtc();
        m_saveTimer.start();
    });
    connect(&m_controller, &Controller::shuffleChanged, this, schedule);
    connect(&m_controller, &Controller::repeatChanged, this, schedule);
    connect(&m_controller, &Controller::volumeChanged, this, schedule);
    connect(&m_controller, &Controller::mutedChanged, this, schedule);
}

SessionStore::~SessionStore()
{
    save();
}

void SessionStore::restore()
{
    QFile file(m_path);
    if (!file.open(QIODevice::ReadOnly))
        return;

    const QJsonObject session = QJsonDocument::fromJson(file.readAll()).object();
    const QJsonValue playedAt = session.value(QStringLiteral("playedAt"));
    m_savedAt = QFileInfo(m_path).lastModified().toUTC();
    if (!playedAt.isUndefined())
        m_playedAt = QDateTime::fromSecsSinceEpoch(core::json::toInt(playedAt), QTimeZone::UTC);
    QList<media::Track> tracks;
    const QJsonArray stored = session.value(QStringLiteral("tracks")).toArray();
    for (const QJsonValue &entry : stored) {
        const media::Track track = decode(entry.toObject());
        if (track.valid())
            tracks.append(track);
    }
    if (tracks.isEmpty())
        return;

    m_controller.setVolume(int(core::json::toInt(session.value(QStringLiteral("volume")), 100)));
    m_controller.setMuted(session.value(QStringLiteral("muted")).toBool());
    m_controller.setShuffle(session.value(QStringLiteral("shuffle")).toBool());
    m_controller.setRepeat(static_cast<media::PlaybackController::RepeatMode>(
        qBound(0LL, core::json::toInt(session.value(QStringLiteral("repeat"))), 2LL)));
    m_controller.restoreQueue(tracks,
                              int(core::json::toInt(session.value(QStringLiteral("index")))),
                              decodeSource(session.value(QStringLiteral("source")).toObject()));
    m_controller.resumeAt(core::json::toInt(session.value(QStringLiteral("position"))));

    qCInfo(logPlatform) << "restored" << tracks.size() << "queued tracks";
}

void SessionStore::save()
{
    const QList<media::Track> &queue = m_controller.queue();
    if (queue.isEmpty()) {
        QFile::remove(m_path);
        return;
    }

    const int index = m_controller.queueIndex();
    const int first =
        qBound(0, index - kMaxStoredTracks / 2, qMax(0, int(queue.size()) - kMaxStoredTracks));
    QJsonArray tracks;
    for (const media::Track &track : queue.mid(first, kMaxStoredTracks))
        tracks.append(encode(track));

    const qint64 position = m_controller.position();
    const qint64 duration = m_controller.duration();
    const bool resumable = !m_controller.live() && (duration <= 0 || position + kTailMs < duration);

    QJsonObject session {
        {QStringLiteral("tracks"), tracks},
        {QStringLiteral("index"), qMax(0, index - first)},
        {QStringLiteral("position"), double(resumable ? position : 0)},
        {QStringLiteral("shuffle"), m_controller.shuffle()},
        {QStringLiteral("repeat"), m_controller.repeat()},
        {QStringLiteral("volume"), m_controller.volume()},
        {QStringLiteral("muted"), m_controller.muted()},
    };
    if (m_controller.playing())
        m_playedAt = QDateTime::currentDateTimeUtc();
    if (m_playedAt.isValid())
        session.insert(QStringLiteral("playedAt"), double(m_playedAt.toSecsSinceEpoch()));
    if (m_controller.queueSource().valid())
        session.insert(QStringLiteral("source"), encodeSource(m_controller.queueSource()));

    QSaveFile file(m_path);
    if (!file.open(QIODevice::WriteOnly)) {
        qCWarning(logPlatform) << "cannot persist the session to" << m_path;
        return;
    }
    file.write(QJsonDocument(session).toJson(QJsonDocument::Compact));
    file.commit();
}

}
