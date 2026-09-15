#include "PlayLog.h"

#include "LibraryActions.h"
#include "TrackStorage.h"
#include "core/Paths.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>

namespace {

QString logPath()
{
    return core::paths::dataDir() + QStringLiteral("/play-log.json");
}

}

namespace library {

PlayLog::PlayLog(QObject *parent)
    : QObject(parent)
{
    load();
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &PlayLog::finish);
    connect(&LibraryActions::instance(), &LibraryActions::trackRatingChanged, this,
            &PlayLog::setRating);
}

PlayLog &PlayLog::instance()
{
    static auto *log = new PlayLog(QCoreApplication::instance());
    return *log;
}

void PlayLog::begin(const media::Track &track)
{
    finish();
    m_track = track;
    m_reached = 0;
    m_started = false;
}

void PlayLog::reach(qint64 position, qint64 duration)
{
    if (!m_track.valid() || duration <= 0 || position <= 0)
        return;
    m_started = true;
    m_reached = qMax(m_reached, qBound(0.0, double(position) / duration, 1.0));
}

void PlayLog::finish()
{
    if (m_track.valid() && m_started) {
        Entry &entry = m_entries[m_track.videoId];
        entry.track = m_track;
        const int count = entry.plays + entry.skips;
        entry.completion = (entry.completion * count + m_reached) / (count + 1);
        if (m_reached < 1.0 / 3)
            ++entry.skips;
        else
            ++entry.plays;
        entry.lastPlayed = QDateTime::currentSecsSinceEpoch();
        save();
    }
    m_track = {};
    m_reached = 0;
    m_started = false;
}

void PlayLog::setRating(const QString &videoId, int rating)
{
    if (m_track.videoId == videoId)
        m_track.liked = rating > 0;
    const auto found = m_entries.find(videoId);
    if (found == m_entries.end() || found->track.liked == (rating > 0))
        return;
    found->track.liked = rating > 0;
    save();
}

void PlayLog::load()
{
    QFile file(logPath());
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    if (root.value(QStringLiteral("version")).toInt() != 1)
        return;
    for (const QJsonValue &value : root.value(QStringLiteral("entries")).toArray()) {
        const QJsonObject object = value.toObject();
        Entry entry;
        entry.track = trackStorage::decode(object);
        if (!entry.track.valid())
            continue;
        entry.plays = qMax(0, object.value(QStringLiteral("plays")).toInt());
        entry.skips = qMax(0, object.value(QStringLiteral("skips")).toInt());
        entry.completion = qBound(0.0, object.value(QStringLiteral("completion")).toDouble(), 1.0);
        entry.lastPlayed = qint64(object.value(QStringLiteral("lastPlayed")).toDouble());
        m_entries.insert(entry.track.videoId, entry);
    }
}

void PlayLog::save() const
{
    QJsonArray entries;
    for (const Entry &entry : m_entries) {
        QJsonObject object = trackStorage::encode(entry.track);
        object.insert(QStringLiteral("plays"), entry.plays);
        object.insert(QStringLiteral("skips"), entry.skips);
        object.insert(QStringLiteral("completion"), entry.completion);
        object.insert(QStringLiteral("lastPlayed"), double(entry.lastPlayed));
        entries.append(object);
    }
    QSaveFile file(logPath());
    if (!file.open(QIODevice::WriteOnly))
        return;
    file.write(QJsonDocument(QJsonObject {{QStringLiteral("version"), 1},
                                          {QStringLiteral("entries"), entries}})
                   .toJson(QJsonDocument::Compact));
    file.commit();
}

}
