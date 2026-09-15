#include "VideoSurface.h"

#include "VideoOutput.h"
#include "core/Logging.h"

#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QQuickOpenGLUtils>
#include <QQuickWindow>
#include <QSGRendererInterface>

#include <mpv/render_gl.h>

namespace {

void *resolveOpenGl(void *, const char *name)
{
    QOpenGLContext *context = QOpenGLContext::currentContext();
    return context ? reinterpret_cast<void *>(context->getProcAddress(name)) : nullptr;
}

void notifyFrame(void *)
{
    QMetaObject::invokeMethod(&player::VideoOutput::instance(), &player::VideoOutput::requestFrame,
                              Qt::QueuedConnection);
}

class SurfaceRenderer : public QQuickFramebufferObject::Renderer
{
public:
    ~SurfaceRenderer() override { adopt(nullptr); }

    void synchronize(QQuickFramebufferObject *) override
    {
        adopt(player::VideoOutput::instance().supported() ? player::VideoOutput::instance().source()
                                                          : nullptr);
    }

    void render() override
    {
        QOpenGLFramebufferObject *target = framebufferObject();
        if (!target)
            return;

        if (!m_context) {
            clear();
            return;
        }

        mpv_opengl_fbo frame {int(target->handle()), target->width(), target->height(), 0};
        int flip = 0;
        mpv_render_param parameters[] = {
            {MPV_RENDER_PARAM_OPENGL_FBO, &frame},
            {MPV_RENDER_PARAM_FLIP_Y, &flip},
            {MPV_RENDER_PARAM_INVALID, nullptr},
        };
        mpv_render_context_render(m_context, parameters);
        QQuickOpenGLUtils::resetOpenGLState();
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override
    {
        return new QOpenGLFramebufferObject(size, QOpenGLFramebufferObject::NoAttachment);
    }

private:
    static void clear()
    {
        QOpenGLContext *context = QOpenGLContext::currentContext();
        if (!context)
            return;
        QOpenGLFunctions *functions = context->functions();
        functions->glClearColor(0, 0, 0, 0);
        functions->glClear(GL_COLOR_BUFFER_BIT);
    }

    void adopt(mpv_handle *handle)
    {
        if (m_handle == handle)
            return;

        if (m_context) {
            mpv_render_context_set_update_callback(m_context, nullptr, nullptr);
            mpv_render_context_free(m_context);
            m_context = nullptr;
        }
        m_handle = handle;

        if (m_handle) {
            mpv_opengl_init_params opengl {&resolveOpenGl, nullptr};
            mpv_render_param parameters[] = {
                {MPV_RENDER_PARAM_API_TYPE,
                 const_cast<char *>(static_cast<const char *>(MPV_RENDER_API_TYPE_OPENGL))},
                {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &opengl},
                {MPV_RENDER_PARAM_INVALID, nullptr},
            };
            const int status = mpv_render_context_create(&m_context, m_handle, parameters);
            if (status < 0) {
                qCWarning(logPlayback) << "video surface" << mpv_error_string(status);
                m_context = nullptr;
            } else {
                mpv_render_context_set_update_callback(m_context, &notifyFrame, nullptr);
            }
        }

        mpv_handle *adopted = m_context ? m_handle : nullptr;
        QMetaObject::invokeMethod(&player::VideoOutput::instance(), [adopted] {
            player::VideoOutput::instance().adoptRendering(adopted);
        }, Qt::QueuedConnection);
    }

    mpv_handle *m_handle = nullptr;
    mpv_render_context *m_context = nullptr;
};

}

namespace player {

VideoSurface::VideoSurface(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
{
    setTextureFollowsItemSize(true);
    connect(&VideoOutput::instance(), &VideoOutput::frameRequested, this, &QQuickItem::update);
    connect(&VideoOutput::instance(), &VideoOutput::sourceChanged, this, &QQuickItem::update);
}

VideoSurface::~VideoSurface()
{
    if (m_holding)
        VideoOutput::instance().release();
}

QQuickFramebufferObject::Renderer *VideoSurface::createRenderer() const
{
    return new SurfaceRenderer;
}

void VideoSurface::itemChange(ItemChange change, const ItemChangeData &data)
{
    QQuickFramebufferObject::itemChange(change, data);

    if (change != ItemSceneChange)
        return;

    publishSupport();
    watchWindow();
    updateHold();
}

void VideoSurface::watchWindow()
{
    QObject::disconnect(m_windowWatch);
    QQuickWindow *host = window();
    if (!host)
        return;
    m_windowWatch = connect(host, &QWindow::visibleChanged, this, &VideoSurface::updateHold);
}

void VideoSurface::updateHold()
{
    const bool holding = window() && window()->isVisible();
    if (m_holding == holding)
        return;

    m_holding = holding;
    if (m_holding)
        VideoOutput::instance().hold();
    else
        VideoOutput::instance().release();
}

void VideoSurface::publishSupport() const
{
    QQuickWindow *host = window();
    if (!host || !host->rendererInterface())
        return;
    VideoOutput::instance().setSupported(host->rendererInterface()->graphicsApi()
                                         == QSGRendererInterface::OpenGL);
}

}
