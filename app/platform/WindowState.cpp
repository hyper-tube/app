#include "WindowState.h"

#include "core/Logging.h"

#include <QGuiApplication>
#include <QQuickWindow>
#include <QScreen>
#include <QSettings>

namespace {

constexpr int kSaveDelayMs = 1000;
constexpr int kDefaultWidth = 1400;
constexpr int kDefaultHeight = 900;
constexpr int kMinimumWidth = 1040;
constexpr int kMinimumHeight = 680;

const QString kXKey = QStringLiteral("window/x");
const QString kYKey = QStringLiteral("window/y");
const QString kWidthKey = QStringLiteral("window/width");
const QString kHeightKey = QStringLiteral("window/height");
const QString kMaximizedKey = QStringLiteral("window/maximized");

QRect centeredOnPrimary()
{
    const QScreen *primary = QGuiApplication::primaryScreen();
    const QRect available =
        primary ? primary->availableGeometry() : QRect(0, 0, kDefaultWidth, kDefaultHeight);
    const QSize size(qMin(kDefaultWidth, available.width()),
                     qMin(kDefaultHeight, available.height()));
    return QRect(available.center() - QPoint(size.width() / 2, size.height() / 2), size);
}

const QScreen *screenHolding(const QRect &geometry)
{
    const QScreen *host = nullptr;
    qint64 covered = 0;
    for (const QScreen *screen : QGuiApplication::screens()) {
        const QRect shared = screen->availableGeometry().intersected(geometry);
        const qint64 area = qint64(shared.width()) * shared.height();
        if (area > covered) {
            covered = area;
            host = screen;
        }
    }
    return host;
}

QRect fitToScreen(const QRect &wanted)
{
    const QScreen *host = screenHolding(wanted);
    if (!host)
        return centeredOnPrimary();

    const QRect available = host->availableGeometry();
    QRect geometry(wanted);
    geometry.setWidth(
        qBound(kMinimumWidth, geometry.width(), qMax(kMinimumWidth, available.width())));
    geometry.setHeight(
        qBound(kMinimumHeight, geometry.height(), qMax(kMinimumHeight, available.height())));

    if (geometry.right() > available.right())
        geometry.moveRight(available.right());
    if (geometry.bottom() > available.bottom())
        geometry.moveBottom(available.bottom());
    if (geometry.left() < available.left())
        geometry.moveLeft(available.left());
    if (geometry.top() < available.top())
        geometry.moveTop(available.top());

    return geometry;
}

QRect storedGeometry(const QSettings &settings)
{
    const QRect fallback = centeredOnPrimary();
    return QRect(settings.value(kXKey, fallback.x()).toInt(),
                 settings.value(kYKey, fallback.y()).toInt(),
                 settings.value(kWidthKey, fallback.width()).toInt(),
                 settings.value(kHeightKey, fallback.height()).toInt());
}

}

namespace platform {

WindowState::WindowState(QObject *parent)
    : QObject(parent)
{
    const QSettings settings;
    m_restored = fitToScreen(storedGeometry(settings));
    m_current = m_restored;
    m_maximized = settings.value(kMaximizedKey, false).toBool();
    qCDebug(logPlatform) << "window restored at" << m_restored << "maximized" << m_maximized;

    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(kSaveDelayMs);
    connect(&m_saveTimer, &QTimer::timeout, this, &WindowState::save);
}

WindowState &WindowState::instance()
{
    static auto *state = new WindowState(QCoreApplication::instance());
    return *state;
}

WindowState *WindowState::create(QQmlEngine *, QJSEngine *)
{
    QQmlEngine::setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
    return &instance();
}

void WindowState::bind(QQuickWindow *window)
{
    if (!window || m_window)
        return;

    m_window = window;
    adoptRestored();
    if (m_maximized)
        m_window->setVisibility(QWindow::Maximized);

    connect(m_window, &QWindow::xChanged, this, &WindowState::record);
    connect(m_window, &QWindow::yChanged, this, &WindowState::record);
    connect(m_window, &QWindow::widthChanged, this, &WindowState::record);
    connect(m_window, &QWindow::heightChanged, this, &WindowState::record);
    connect(m_window, &QWindow::visibilityChanged, this, &WindowState::record);
    connect(qGuiApp, &QGuiApplication::screenRemoved, this, &WindowState::keepOnScreen,
            Qt::QueuedConnection);
}

void WindowState::adoptRestored()
{
    m_window->setGeometry(m_restored);
}

void WindowState::record()
{
    const QWindow::Visibility visibility = m_window->visibility();
    if (visibility == QWindow::Hidden || visibility == QWindow::Minimized)
        return;

    m_maximized = visibility != QWindow::Windowed;
    if (!m_maximized) {
        const QRect geometry = m_window->geometry();
        if (geometry.width() >= m_window->minimumWidth()
            && geometry.height() >= m_window->minimumHeight())
            m_current = geometry;
    }
    m_saveTimer.start();
}

void WindowState::save()
{
    QSettings settings;
    settings.setValue(kXKey, m_current.x());
    settings.setValue(kYKey, m_current.y());
    settings.setValue(kWidthKey, m_current.width());
    settings.setValue(kHeightKey, m_current.height());
    settings.setValue(kMaximizedKey, m_maximized);
}

void WindowState::keepOnScreen()
{
    if (!m_window || m_window->visibility() != QWindow::Windowed)
        return;

    const QRect corrected = fitToScreen(m_window->geometry());
    if (corrected != m_window->geometry())
        m_window->setGeometry(corrected);
}

}
