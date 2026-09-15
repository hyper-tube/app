#include "VideoOutput.h"

#include "core/Logging.h"

#include <QCoreApplication>

namespace {

constexpr int kReleaseGraceMs = 700;

}

namespace player {

VideoOutput::VideoOutput(QObject *parent)
    : QObject(parent)
{
    m_grace.setSingleShot(true);
    m_grace.setInterval(kReleaseGraceMs);
    connect(&m_grace, &QTimer::timeout, this, &VideoOutput::settle);
}

VideoOutput &VideoOutput::instance()
{
    static auto *output = new VideoOutput(QCoreApplication::instance());
    return *output;
}

void VideoOutput::setSource(mpv_handle *handle)
{
    if (m_source == handle)
        return;
    m_source = handle;
    Q_EMIT sourceChanged();
}

void VideoOutput::setSupported(bool supported)
{
    if (m_supported == supported)
        return;
    m_supported = supported;
    if (!supported)
        qCWarning(logPlayback) << "the scene graph is not using OpenGL, video playback is off";
    Q_EMIT supportedChanged();
}

void VideoOutput::setWanted(bool wanted)
{
    if (m_wanted == wanted)
        return;
    m_wanted = wanted;
    refresh();
}

void VideoOutput::hold()
{
    ++m_holders;
    refresh();
}

void VideoOutput::release()
{
    if (m_holders > 0)
        --m_holders;
    refresh();
}

void VideoOutput::refresh()
{
    const bool desired = m_wanted && m_holders > 0 && m_supported;
    if (desired == m_active) {
        m_grace.stop();
        return;
    }
    if (!desired) {
        m_grace.start();
        return;
    }
    m_grace.stop();
    m_active = true;
    Q_EMIT activeChanged();
}

void VideoOutput::settle()
{
    if (m_wanted && m_holders > 0 && m_supported)
        return;
    m_active = false;
    Q_EMIT activeChanged();
}

void VideoOutput::adoptRendering(mpv_handle *handle)
{
    if (m_rendering == handle)
        return;
    m_rendering = handle;
    Q_EMIT renderingChanged();
}

void VideoOutput::requestFrame()
{
    Q_EMIT frameRequested();
}

}
