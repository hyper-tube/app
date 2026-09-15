#pragma once

#include <QMetaType>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace media {

class Track
{
    Q_GADGET
    QML_VALUE_TYPE(track)

    Q_PROPERTY(QString videoId MEMBER videoId)
    Q_PROPERTY(QString title MEMBER title)
    Q_PROPERTY(QString artist MEMBER artist)
    Q_PROPERTY(QString album MEMBER album)
    Q_PROPERTY(QString artId MEMBER artId)
    Q_PROPERTY(qint64 durationMs MEMBER durationMs)
    Q_PROPERTY(bool liked MEMBER liked)
    Q_PROPERTY(bool disliked MEMBER disliked)
    Q_PROPERTY(bool video MEMBER video)
    Q_PROPERTY(bool upload MEMBER upload)
    Q_PROPERTY(bool live MEMBER live)
    Q_PROPERTY(bool episode MEMBER episode)
    Q_PROPERTY(bool valid READ valid)

public:
    QString videoId;
    QString title;
    QString artist;
    QString album;
    QString artId;
    qint64 durationMs = 0;
    bool liked = false;
    bool disliked = false;
    bool ratingKnown = false;
    bool video = false;
    bool upload = false;
    bool live = false;
    bool episode = false;

    bool valid() const { return !videoId.isEmpty(); }
    bool operator==(const Track &other) const { return videoId == other.videoId; }
};

}

Q_DECLARE_METATYPE(media::Track)
