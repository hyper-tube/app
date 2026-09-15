#pragma once

#include <QObject>

class QQuickWindow;

namespace media {
class PlaybackController;
}

namespace platform {

class MediaControls;
class TrayIcon;

class Desktop : public QObject
{
    Q_OBJECT

public:
    Desktop(media::PlaybackController &controller, QQuickWindow *window, QObject *parent = nullptr);

    void raise();
    void toggleWindow();

private:
    void reopen(Qt::ApplicationState state);

    QQuickWindow *m_window;
    MediaControls *m_media;
    TrayIcon *m_tray;
};

}
