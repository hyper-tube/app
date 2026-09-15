#pragma once

#include <QMetaType>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace model {

class ItemActions
{
    Q_GADGET
    QML_VALUE_TYPE(itemActions)

    Q_PROPERTY(QString mixPlaylistId MEMBER mixPlaylistId)
    Q_PROPERTY(QString mixVideoId MEMBER mixVideoId)
    Q_PROPERTY(QString shufflePlaylistId MEMBER shufflePlaylistId)
    Q_PROPERTY(QString artistId MEMBER artistId)
    Q_PROPERTY(QString albumId MEMBER albumId)
    Q_PROPERTY(QString podcastId MEMBER podcastId)
    Q_PROPERTY(QString episodeId MEMBER episodeId)
    Q_PROPERTY(QString profileId MEMBER profileId)
    Q_PROPERTY(QString setVideoId MEMBER setVideoId)
    Q_PROPERTY(QString sourcePlaylistId MEMBER sourcePlaylistId)
    Q_PROPERTY(QString libraryAddToken MEMBER libraryAddToken)
    Q_PROPERTY(QString libraryRemoveToken MEMBER libraryRemoveToken)
    Q_PROPERTY(QString pinToken MEMBER pinToken)
    Q_PROPERTY(QString unpinToken MEMBER unpinToken)
    Q_PROPERTY(QString playedToken MEMBER playedToken)
    Q_PROPERTY(QString unplayedToken MEMBER unplayedToken)
    Q_PROPERTY(bool inLibrary MEMBER inLibrary)
    Q_PROPERTY(bool editable MEMBER editable)
    Q_PROPERTY(bool laterable MEMBER laterable)
    Q_PROPERTY(bool later MEMBER later)
    Q_PROPERTY(bool markable READ markable)
    Q_PROPERTY(bool mixable READ mixable)
    Q_PROPERTY(bool pinnable READ pinnable)
    Q_PROPERTY(bool collectable READ collectable)
    Q_PROPERTY(bool removable READ removable)

public:
    QString mixPlaylistId;
    QString mixVideoId;
    QString shufflePlaylistId;
    QString artistId;
    QString albumId;
    QString podcastId;
    QString episodeId;
    QString profileId;
    QString setVideoId;
    QString sourcePlaylistId;
    QString libraryAddToken;
    QString libraryRemoveToken;
    QString pinToken;
    QString unpinToken;
    QString playedToken;
    QString unplayedToken;
    bool inLibrary = false;
    bool editable = false;
    bool laterable = false;
    bool later = false;

    bool mixable() const { return !mixPlaylistId.isEmpty(); }
    bool pinnable() const { return !pinToken.isEmpty() || !unpinToken.isEmpty(); }
    bool collectable() const { return !libraryAddToken.isEmpty() || !libraryRemoveToken.isEmpty(); }
    bool removable() const { return !setVideoId.isEmpty() && !sourcePlaylistId.isEmpty(); }
    bool markable() const { return !playedToken.isEmpty() || !unplayedToken.isEmpty(); }

    bool operator==(const ItemActions &) const = default;
};

}

Q_DECLARE_METATYPE(model::ItemActions)
