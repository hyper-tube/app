#pragma once

#include <QObject>
#include <QTimer>

struct mpv_handle;

namespace player {

class VideoOutput : public QObject
{
    Q_OBJECT

public:
    explicit VideoOutput(QObject *parent);

    static VideoOutput &instance();

    mpv_handle *source() const { return m_source; }
    mpv_handle *rendering() const { return m_rendering; }
    bool supported() const { return m_supported; }
    bool active() const { return m_active; }

    void setSource(mpv_handle *handle);
    void setSupported(bool supported);
    void setWanted(bool wanted);
    void hold();
    void release();
    void adoptRendering(mpv_handle *handle);

public Q_SLOTS:
    void requestFrame();

Q_SIGNALS:
    void sourceChanged();
    void supportedChanged();
    void activeChanged();
    void renderingChanged();
    void frameRequested();

private:
    void refresh();
    void settle();

    QTimer m_grace;
    mpv_handle *m_source = nullptr;
    mpv_handle *m_rendering = nullptr;
    int m_holders = 0;
    bool m_wanted = false;
    bool m_active = false;
    bool m_supported = true;
};

}
