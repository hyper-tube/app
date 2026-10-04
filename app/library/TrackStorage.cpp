#include "TrackStorage.h"

namespace library::trackStorage {

QJsonObject encode(const media::Track &track)
{
    return {{QStringLiteral("videoId"), track.videoId},
            {QStringLiteral("title"), track.title},
            {QStringLiteral("artist"), track.artist},
            {QStringLiteral("album"), track.album},
            {QStringLiteral("albumId"), track.albumId},
            {QStringLiteral("credits"), encodeCredits(track.credits)},
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
    track.albumId = object.value(QStringLiteral("albumId")).toString();
    track.credits = decodeCredits(object.value(QStringLiteral("credits")));
    track.artId = object.value(QStringLiteral("artId")).toString();
    track.durationMs = qint64(object.value(QStringLiteral("durationMs")).toDouble());
    track.liked = object.value(QStringLiteral("liked")).toBool();
    track.video = object.value(QStringLiteral("video")).toBool();
    track.upload = object.value(QStringLiteral("upload")).toBool();
    track.episode = object.value(QStringLiteral("episode")).toBool();
    return track;
}

QJsonArray encodeCredits(const QList<media::Credit> &credits)
{
    QJsonArray encoded;
    for (const media::Credit &credit : credits) {
        encoded.append(QJsonObject {{QStringLiteral("name"), credit.name},
                                    {QStringLiteral("browseId"), credit.browseId},
                                    {QStringLiteral("kind"), credit.kind}});
    }
    return encoded;
}

QList<media::Credit> decodeCredits(const QJsonValue &value)
{
    QList<media::Credit> credits;
    for (const QJsonValue &entry : value.toArray()) {
        const QJsonObject object = entry.toObject();
        const media::Credit credit {object.value(QStringLiteral("name")).toString(),
                                    object.value(QStringLiteral("browseId")).toString(),
                                    object.value(QStringLiteral("kind")).toString()};
        if (!credit.name.isEmpty() && !credit.browseId.isEmpty())
            credits.append(credit);
    }
    return credits;
}

}
