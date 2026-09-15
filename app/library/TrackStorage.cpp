#include "TrackStorage.h"

namespace library::trackStorage {

QJsonObject encode(const media::Track &track)
{
    return {{QStringLiteral("videoId"), track.videoId},
            {QStringLiteral("title"), track.title},
            {QStringLiteral("artist"), track.artist},
            {QStringLiteral("album"), track.album},
            {QStringLiteral("artId"), track.artId},
            {QStringLiteral("durationMs"), double(track.durationMs)},
            {QStringLiteral("liked"), track.liked},
            {QStringLiteral("video"), track.video},
            {QStringLiteral("upload"), track.upload},
            {QStringLiteral("episode"), track.episode}};
}

media::Track decode(const QJsonObject &object)
{
    media::Track track;
    track.videoId = object.value(QStringLiteral("videoId")).toString();
    track.title = object.value(QStringLiteral("title")).toString();
    track.artist = object.value(QStringLiteral("artist")).toString();
    track.album = object.value(QStringLiteral("album")).toString();
    track.artId = object.value(QStringLiteral("artId")).toString();
    track.durationMs = qint64(object.value(QStringLiteral("durationMs")).toDouble());
    track.liked = object.value(QStringLiteral("liked")).toBool();
    track.video = object.value(QStringLiteral("video")).toBool();
    track.upload = object.value(QStringLiteral("upload")).toBool();
    track.episode = object.value(QStringLiteral("episode")).toBool();
    return track;
}

}
