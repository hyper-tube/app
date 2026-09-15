#pragma once

#include <QObject>

class QWindow;

namespace media {
class PlaybackController;
}

namespace platform {

class MediaControls : public QObject
{
    Q_OBJECT

public:
    static MediaControls *create(media::PlaybackController &controller, QWindow *window,
                                 QObject *parent);

    MediaControls(media::PlaybackController &controller, QObject *parent);

    media::PlaybackController &controller() const { return m_controller; }

    void requestRaise();
    void requestQuit();

Q_SIGNALS:
    void raiseRequested();
    void quitRequested();

private:
    media::PlaybackController &m_controller;
};

}
