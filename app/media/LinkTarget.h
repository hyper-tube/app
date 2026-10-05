#pragma once

#include <QString>
#include <QUrl>

namespace media {

struct LinkTarget
{
    QString videoId;
    QString playlistId;
    QUrl page;

    bool playable() const { return !videoId.isEmpty() || !playlistId.isEmpty(); }
    bool valid() const { return playable() || page.isValid(); }

    static LinkTarget parse(const QString &text);
};

}
