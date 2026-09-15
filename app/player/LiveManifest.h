#pragma once

#include <QByteArray>
#include <QUrl>

namespace player {

struct LiveManifest
{
    QUrl audio;
    QUrl picture;
    int pictureHeight = 0;

    bool valid() const { return !audio.isEmpty(); }

    static LiveManifest parse(const QByteArray &playlist, const QUrl &base, int maximumHeight);
};

}
