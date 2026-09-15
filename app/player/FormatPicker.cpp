#include "FormatPicker.h"

#include "core/Json.h"
#include "core/Logging.h"

#include <QJsonObject>

#include <tuple>

namespace {

using player::AudioFormat;
using player::VideoFormat;

const QString kRectangularProjection = QStringLiteral("RECTANGULAR");

int qualityRank(const QString &quality)
{
    if (quality == QLatin1String("AUDIO_QUALITY_HIGH"))
        return 3;
    if (quality == QLatin1String("AUDIO_QUALITY_MEDIUM"))
        return 2;
    if (quality == QLatin1String("AUDIO_QUALITY_LOW"))
        return 1;
    return 0;
}

int codecRank(const QString &mimeType)
{
    if (mimeType.contains(QLatin1String("opus")))
        return 2;
    if (mimeType.contains(QLatin1String("mp4a")))
        return 1;
    return 0;
}

int pictureCodecRank(const QString &mimeType)
{
    if (mimeType.contains(QLatin1String("vp9")))
        return 3;
    if (mimeType.contains(QLatin1String("avc1")))
        return 2;
    if (mimeType.contains(QLatin1String("av01")))
        return 1;
    return 0;
}

bool isAudio(const QJsonObject &format)
{
    return !format.contains(QStringLiteral("width"));
}

bool isCiphered(const QJsonObject &format)
{
    return format.contains(QStringLiteral("signatureCipher"))
        || format.contains(QStringLiteral("cipher"));
}

bool isAutoDubbed(const QJsonObject &format)
{
    return format.value(QStringLiteral("audioTrack"))
        .toObject()
        .value(QStringLiteral("isAutoDubbed"))
        .toBool();
}

AudioFormat read(const QJsonObject &format)
{
    AudioFormat audio;
    audio.itag = int(core::json::toInt(format.value(QStringLiteral("itag"))));
    audio.url = QUrl(format.value(QStringLiteral("url")).toString());
    audio.mimeType = format.value(QStringLiteral("mimeType")).toString();
    audio.quality = format.value(QStringLiteral("audioQuality")).toString();
    audio.bitrate = core::json::toInt(format.value(QStringLiteral("bitrate")));
    audio.channels = int(core::json::toInt(format.value(QStringLiteral("audioChannels")), 2));

    const QJsonValue loudness = format.value(QStringLiteral("loudnessDb"));
    audio.hasLoudness = !loudness.isUndefined() && !loudness.isNull();
    if (audio.hasLoudness)
        audio.loudnessDb = core::json::toDouble(loudness);

    return audio;
}

auto ranking(const AudioFormat &format)
{
    return std::make_tuple(qualityRank(format.quality), format.channels, codecRank(format.mimeType),
                           format.bitrate);
}

bool isFlat(const QJsonObject &format)
{
    const QString projection = format.value(QStringLiteral("projectionType")).toString();
    return projection.isEmpty() || projection == kRectangularProjection;
}

VideoFormat readPicture(const QJsonObject &format)
{
    VideoFormat picture;
    picture.itag = int(core::json::toInt(format.value(QStringLiteral("itag"))));
    picture.url = QUrl(format.value(QStringLiteral("url")).toString());
    picture.mimeType = format.value(QStringLiteral("mimeType")).toString();
    picture.bitrate = core::json::toInt(format.value(QStringLiteral("bitrate")));
    picture.width = int(core::json::toInt(format.value(QStringLiteral("width"))));
    picture.height = int(core::json::toInt(format.value(QStringLiteral("height"))));
    picture.framesPerSecond = int(core::json::toInt(format.value(QStringLiteral("fps")), 30));
    return picture;
}

auto ranking(const VideoFormat &format)
{
    return std::make_tuple(format.height, pictureCodecRank(format.mimeType), format.framesPerSecond,
                           format.bitrate);
}

}

namespace player::formatPicker {

AudioFormat best(const QJsonArray &adaptiveFormats)
{
    AudioFormat winner;
    AudioFormat dubbed;

    for (const QJsonValue &value : adaptiveFormats) {
        const QJsonObject format = value.toObject();
        if (!isAudio(format) || isCiphered(format))
            continue;

        const AudioFormat candidate = read(format);
        if (!candidate.valid())
            continue;

        AudioFormat &target = isAutoDubbed(format) ? dubbed : winner;
        if (!target.valid() || ranking(candidate) > ranking(target))
            target = candidate;
    }

    if (!winner.valid() && dubbed.valid()) {
        qCDebug(logStream) << "only auto-dubbed audio available";
        return dubbed;
    }
    return winner;
}

VideoFormat bestPicture(const QJsonArray &adaptiveFormats, int maximumHeight)
{
    VideoFormat winner;

    for (const QJsonValue &value : adaptiveFormats) {
        const QJsonObject format = value.toObject();
        if (isAudio(format) || isCiphered(format) || !isFlat(format))
            continue;

        const VideoFormat candidate = readPicture(format);
        if (!candidate.valid() || candidate.height <= 0 || candidate.height > maximumHeight)
            continue;
        if (pictureCodecRank(candidate.mimeType) == 0)
            continue;

        if (!winner.valid() || ranking(candidate) > ranking(winner))
            winner = candidate;
    }

    return winner;
}

}
