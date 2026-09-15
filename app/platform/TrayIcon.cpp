#include "TrayIcon.h"

#include "core/AppInfo.h"
#include "core/Localization.h"
#include "core/Logging.h"
#include "media/PlaybackController.h"

#include <QCoreApplication>
#include <QCursor>
#include <QGuiApplication>
#include <QIcon>
#include <QMenu>
#include <QScreen>
#include <QWindow>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {

QIcon themed(const QString &name, const QString &fallback)
{
    return QIcon::fromTheme(name, QIcon::fromTheme(fallback));
}

QString tooltipFor(const media::Track &track)
{
    if (!track.valid())
        return core::AppInfo::name();
    if (track.artist.isEmpty())
        return track.title;
    return track.title + QStringLiteral(" - ") + track.artist;
}

QRect availableAreaAt(const QPoint &point)
{
    const QScreen *screen = QGuiApplication::screenAt(point);
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    return screen ? screen->availableGeometry() : QRect();
}

void raiseToForeground(QWindow *window)
{
#ifdef Q_OS_WIN
    ::SetForegroundWindow(reinterpret_cast<HWND>(window->winId()));
#else
    Q_UNUSED(window)
#endif
}

}

namespace platform {

TrayIcon::TrayIcon(QObject *parent)
    : QObject(parent)
    , m_controller(media::PlaybackController::instance())
    , m_icon(QIcon(QStringLiteral(":/icons/ht-music.svg")), this)
{
    if (!styled()) {
        buildMenu();
        m_icon.setContextMenu(m_menu);
    }

    connect(&m_icon, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger)
            Q_EMIT toggleWindowRequested();
        else if (reason == QSystemTrayIcon::Context && styled())
            openMenu();
    });

    connect(&m_controller, &media::PlaybackController::playingChanged, this, &TrayIcon::refresh);
    connect(&m_controller, &media::PlaybackController::trackChanged, this, &TrayIcon::refresh);
    connect(&core::Localization::instance(), &core::Localization::resolvedChanged, this,
            &TrayIcon::retranslate);
    retranslate();

    if (!QSystemTrayIcon::isSystemTrayAvailable())
        qCInfo(logPlatform) << "no system tray, the icon will appear if one starts later";
    m_icon.show();
}

TrayIcon::~TrayIcon()
{
    delete m_menu;
}

TrayIcon &TrayIcon::instance()
{
    static auto *tray = new TrayIcon(QCoreApplication::instance());
    return *tray;
}

TrayIcon *TrayIcon::create(QQmlEngine *, QJSEngine *)
{
    QQmlEngine::setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
    return &instance();
}

bool TrayIcon::styled()
{
#ifdef Q_OS_WIN
    return true;
#else
    return false;
#endif
}

void TrayIcon::buildMenu()
{
    m_menu = new QMenu;
    m_playPause = m_menu->addAction(QString());
    m_previous = m_menu->addAction(
        themed(QStringLiteral("media-skip-backward"), QStringLiteral("go-previous")), QString());
    m_next = m_menu->addAction(
        themed(QStringLiteral("media-skip-forward"), QStringLiteral("go-next")), QString());

    connect(m_playPause, &QAction::triggered, &m_controller, &media::PlaybackController::toggle);
    connect(m_previous, &QAction::triggered, &m_controller, &media::PlaybackController::previous);
    connect(m_next, &QAction::triggered, &m_controller, &media::PlaybackController::next);

    m_menu->addSeparator();

    m_window = m_menu->addAction(QString());
    connect(m_window, &QAction::triggered, this, &TrayIcon::toggleWindowRequested);

    m_quit = m_menu->addAction(
        themed(QStringLiteral("application-exit"), QStringLiteral("window-close")), QString());
    connect(m_quit, &QAction::triggered, this, &TrayIcon::quitRequested);
}

void TrayIcon::openMenu()
{
    m_anchor = QCursor::pos();
    m_anchorArea = availableAreaAt(m_anchor);
    Q_EMIT menuRequested();
}

void TrayIcon::present(QWindow *menu)
{
    if (!menu)
        return;

    menu->show();
    menu->raise();
    menu->requestActivate();
    raiseToForeground(menu);
}

void TrayIcon::toggleWindow()
{
    Q_EMIT toggleWindowRequested();
}

void TrayIcon::quit()
{
    Q_EMIT quitRequested();
}

void TrayIcon::retranslate()
{
    if (m_menu) {
        m_previous->setText(tr("Previous"));
        m_next->setText(tr("Next"));
        m_window->setText(tr("Show or hide window"));
        m_quit->setText(tr("Quit"));
    }
    refresh();
}

void TrayIcon::refresh()
{
    const bool playing = m_controller.playing();
    if (m_playPause) {
        m_playPause->setText(playing ? tr("Pause") : tr("Play"));
        m_playPause->setIcon(playing ? themed(QStringLiteral("media-playback-pause"),
                                              QStringLiteral("media-playback-stop"))
                                     : themed(QStringLiteral("media-playback-start"),
                                              QStringLiteral("media-playback-stop")));
    }
    m_icon.setToolTip(tooltipFor(m_controller.track()));
}

}
