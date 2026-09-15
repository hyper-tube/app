#include "Desktop.h"

#include "MediaControls.h"
#include "SystemSettings.h"
#include "TrayIcon.h"

#include <QGuiApplication>
#include <QQuickWindow>

namespace platform {

Desktop::Desktop(media::PlaybackController &controller, QQuickWindow *window, QObject *parent)
    : QObject(parent)
    , m_window(window)
    , m_media(MediaControls::create(controller, window, this))
    , m_tray(&TrayIcon::instance())
{
    connect(m_media, &MediaControls::raiseRequested, this, &Desktop::raise);
    connect(m_tray, &TrayIcon::toggleWindowRequested, this, &Desktop::toggleWindow);

    connect(m_media, &MediaControls::quitRequested, qApp, &QCoreApplication::quit);
    connect(m_tray, &TrayIcon::quitRequested, qApp, &QCoreApplication::quit);

    if (SystemSettings::instance().background() == SystemSettings::Dock)
        connect(qApp, &QGuiApplication::applicationStateChanged, this, &Desktop::reopen);
}

void Desktop::raise()
{
    if (!m_window)
        return;

    m_window->show();
    m_window->raise();
    m_window->requestActivate();
}

void Desktop::reopen(Qt::ApplicationState state)
{
    if (state == Qt::ApplicationActive && m_window && !m_window->isVisible())
        raise();
}

void Desktop::toggleWindow()
{
    if (!m_window)
        return;

    if (m_window->isVisible())
        m_window->hide();
    else
        raise();
}

}
