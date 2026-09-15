#include "PlaybackTrackingSeed.h"

#include <QJsonArray>

#include <algorithm>
#include <cmath>

namespace {

constexpr double kMaximumScheduleSeconds = 86400;

qint64 milliseconds(const QJsonValue &value)
{
    const double seconds = value.isString() ? value.toString().toDouble() : value.toDouble();
    if (!std::isfinite(seconds) || seconds <= 0 || seconds > kMaximumScheduleSeconds)
        return 0;
    return qint64(seconds * 1000);
}

QUrl trackingUrl(const QJsonObject &tracking, const QString &key)
{
    const QUrl url(tracking.value(key).toObject().value(QStringLiteral("baseUrl")).toString());
    if (url.scheme() != QLatin1String("https") || !url.userInfo().isEmpty()
        || (url.host() != QLatin1String("youtube.com")
            && !url.host().endsWith(QLatin1String(".youtube.com"))))
        return {};
    return url;
}

}

namespace player {

PlaybackTrackingSeed PlaybackTrackingSeed::fromResponse(const QJsonObject &response,
                                                        const QString &clientKey,
                                                        const QString &clientName)
{
    PlaybackTrackingSeed seed;
    const QJsonObject tracking = response.value(QStringLiteral("playbackTracking")).toObject();
    seed.playbackUrl = trackingUrl(tracking, QStringLiteral("videostatsPlaybackUrl"));
    seed.watchtimeUrl = trackingUrl(tracking, QStringLiteral("videostatsWatchtimeUrl"));
    seed.clientKey = clientKey;
    seed.clientName = clientName;
    for (const QJsonValue &value :
         tracking.value(QStringLiteral("videostatsScheduledFlushWalltimeSeconds")).toArray()) {
        const qint64 offset = milliseconds(value);
        if (offset > 0 && !seed.scheduledFlushMs.contains(offset))
            seed.scheduledFlushMs.append(offset);
    }
    std::ranges::sort(seed.scheduledFlushMs);
    seed.defaultFlushMs =
        milliseconds(tracking.value(QStringLiteral("videostatsDefaultFlushIntervalSeconds")));
    seed.durationMs = milliseconds(response.value(QStringLiteral("videoDetails"))
                                       .toObject()
                                       .value(QStringLiteral("lengthSeconds")));
    return seed;
}

}