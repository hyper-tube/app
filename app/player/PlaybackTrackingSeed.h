#pragma once

#include <QJsonObject>
#include <QList>
#include <QString>
#include <QUrl>

namespace player {

struct PlaybackTrackingSeed
{
    QUrl playbackUrl;
    QUrl watchtimeUrl;
    QString clientKey;
    QString clientName;
    QList<qint64> scheduledFlushMs;
    qint64 defaultFlushMs = 0;
    qint64 durationMs = 0;

    static PlaybackTrackingSeed fromResponse(const QJsonObject &response, const QString &clientKey,
                                             const QString &clientName);
};

}