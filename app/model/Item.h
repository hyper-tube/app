#pragma once

#include "ItemActions.h"
#include "media/Track.h"

#include <QString>
#include <QtQml/qqmlregistration.h>

namespace model {

class Item
{
    Q_GADGET
    QML_VALUE_TYPE(contentItem)

    Q_PROPERTY(QString title MEMBER title)
    Q_PROPERTY(QString subtitle MEMBER subtitle)
    Q_PROPERTY(QString description MEMBER description)
    Q_PROPERTY(QString artId MEMBER artId)
    Q_PROPERTY(QString kind MEMBER kind)
    Q_PROPERTY(QString browseId MEMBER browseId)
    Q_PROPERTY(QString params MEMBER params)
    Q_PROPERTY(QString playlistId MEMBER playlistId)
    Q_PROPERTY(QString channelId MEMBER channelId)
    Q_PROPERTY(QString privacy MEMBER privacy)
    Q_PROPERTY(QString glyph MEMBER glyph)
    Q_PROPERTY(int rank MEMBER rank)
    Q_PROPERTY(QString movement MEMBER movement)
    Q_PROPERTY(QString rankLabel MEMBER rankLabel)
    Q_PROPERTY(QString byline MEMBER byline)
    Q_PROPERTY(QString bylineArtId MEMBER bylineArtId)
    Q_PROPERTY(QString progressLabel MEMBER progressLabel)
    Q_PROPERTY(int progress MEMBER progress)
    Q_PROPERTY(bool played MEMBER played)
    Q_PROPERTY(media::Track track MEMBER track)
    Q_PROPERTY(model::ItemActions actions MEMBER actions)
    Q_PROPERTY(bool saved MEMBER saved)
    Q_PROPERTY(QString savePlaylistId MEMBER savePlaylistId)
    Q_PROPERTY(bool subscribed MEMBER subscribed)
    Q_PROPERTY(bool deletable MEMBER deletable)
    Q_PROPERTY(bool owned MEMBER owned)
    Q_PROPERTY(bool pinned MEMBER pinned)
    Q_PROPERTY(bool striped MEMBER striped)
    Q_PROPERTY(bool circular READ circular)
    Q_PROPERTY(bool episode READ episode)
    Q_PROPERTY(bool playable READ playable)
    Q_PROPERTY(bool valid READ valid)
    Q_PROPERTY(QString playlistTarget READ playlistTarget)
    Q_PROPERTY(QString key READ key)
    Q_PROPERTY(QString shareUrl READ shareUrl)

public:
    QString title;
    QString subtitle;
    QString description;
    QString artId;
    QString kind;
    QString browseId;
    QString params;
    QString playlistId;
    QString channelId;
    QString privacy;
    QString glyph;
    QString movement;
    QString rankLabel;
    QString byline;
    QString bylineArtId;
    QString progressLabel;
    int rank = 0;
    int progress = 0;
    bool played = false;
    QString savePlaylistId;
    media::Track track;
    ItemActions actions;
    bool saved = false;
    bool subscribed = false;
    bool deletable = false;
    bool owned = false;
    bool pinned = false;
    bool striped = false;

    bool circular() const
    {
        return kind == QLatin1String("artist") || kind == QLatin1String("profile");
    }
    bool episode() const { return track.episode; }
    bool playable() const { return track.valid(); }

    QString playlistTarget() const
    {
        const QString id = browseId.isEmpty() ? playlistId : browseId;
        if (id.startsWith(QLatin1String("MPSP")))
            return id.mid(4);
        return id.startsWith(QLatin1String("VL")) ? id.mid(2) : id;
    }

    QString key() const { return playable() ? track.videoId : playlistTarget(); }

    QString shareUrl() const
    {
        static const QString base = QStringLiteral("https://music.youtube.com/");
        if (playable())
            return base + QStringLiteral("watch?v=") + track.videoId;
        if (circular() && !browseId.isEmpty())
            return base + QStringLiteral("channel/") + browseId;
        if (kind == QLatin1String("podcast") && !playlistTarget().isEmpty())
            return base + QStringLiteral("playlist?list=") + playlistTarget();
        if (kind == QLatin1String("album") && !browseId.isEmpty())
            return base + QStringLiteral("browse/") + browseId;
        const QString id = playlistTarget();
        return id.isEmpty() ? QString() : base + QStringLiteral("playlist?list=") + id;
    }

    bool valid() const
    {
        return !title.isEmpty()
            && (playable() || !browseId.isEmpty() || !playlistId.isEmpty()
                || kind == QLatin1String("search"));
    }
};

}

Q_DECLARE_METATYPE(model::Item)
