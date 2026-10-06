#pragma once

#include <QJsonArray>
#include <QString>
#include <QUrl>

namespace player {

struct Cipher
{
    QString signature;
    QString parameter;

    bool present() const { return !signature.isEmpty(); }
};

struct AudioFormat
{
    int itag = 0;
    QUrl url;
    Cipher cipher;
    QString mimeType;
    QString quality;
    qint64 bitrate = 0;
    int channels = 0;
    double loudnessDb = 0.0;
    bool hasLoudness = false;

    bool valid() const { return !url.isEmpty(); }
};

struct VideoFormat
{
    int itag = 0;
    QUrl url;
    Cipher cipher;
    QString mimeType;
    qint64 bitrate = 0;
    int width = 0;
    int height = 0;
    int framesPerSecond = 0;

    bool valid() const { return !url.isEmpty(); }
};

namespace formatPicker {

enum class Ciphered {
    Skip,
    Accept,
};

AudioFormat best(const QJsonArray &adaptiveFormats, Ciphered ciphered);
VideoFormat bestPicture(const QJsonArray &adaptiveFormats, int maximumHeight, Ciphered ciphered);

}

}
