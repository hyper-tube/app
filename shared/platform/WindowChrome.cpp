#include "WindowChrome.h"

#include "WindowFrame.h"
#include "core/Logging.h"

#include <QCoreApplication>
#include <QQuickWindow>
#include <QSettings>

namespace {

constexpr int kBarHeight = 48;
constexpr int kGripThickness = 8;

#if defined(HT_MUSIC_UNIFIED_TITLE_BAR)
constexpr int kNativeTitleArea = 28;
constexpr int kNativeLeadingInset = 78;
#else
constexpr int kNativeTitleArea = 0;
constexpr int kNativeLeadingInset = 0;
#endif

const QString kDecorationsKey = QStringLiteral("window/decorations");
const QString kSystem = QStringLiteral("system");
const QString kCustom = QStringLiteral("custom");

platform::WindowChrome::Decorations storedDecorations()
{
    const QString stored = QSettings().value(kDecorationsKey, kCustom).toString();
    return stored == kSystem ? platform::WindowChrome::System : platform::WindowChrome::Custom;
}

bool drawsOwnTitleBar(platform::WindowChrome::Decorations decorations)
{
#if defined(HT_MUSIC_UNIFIED_TITLE_BAR)
    Q_UNUSED(decorations)
    return false;
#elif defined(HT_MUSIC_NON_CLIENT_FRAME)
    Q_UNUSED(decorations)
    return true;
#else
    return decorations == platform::WindowChrome::Custom;
#endif
}

}

namespace platform {

WindowChrome::WindowChrome(QObject *parent)
    : QObject(parent)
    , m_decorations(storedDecorations())
    , m_applied(m_decorations)
    , m_custom(drawsOwnTitleBar(m_applied))
{
    qCDebug(logPlatform) << "window chrome custom" << m_custom << "selectable" << selectable();
}

WindowChrome &WindowChrome::instance()
{
    static auto *chrome = new WindowChrome(QCoreApplication::instance());
    return *chrome;
}

WindowChrome *WindowChrome::create(QQmlEngine *, QJSEngine *)
{
    QQmlEngine::setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
    return &instance();
}

void WindowChrome::bind(QQuickWindow *window)
{
    if (!window || m_window)
        return;

    m_window = window;
    m_frame = WindowFrame::create(window, this);

    connect(m_frame, &WindowFrame::snapHoveredChanged, this, [this](bool hovered) {
        m_snapHovered = hovered;
        Q_EMIT snapStateChanged();
    });
    connect(m_frame, &WindowFrame::snapPressedChanged, this, [this](bool pressed) {
        m_snapPressed = pressed;
        Q_EMIT snapStateChanged();
    });
    connect(m_frame, &WindowFrame::snapTriggered, this, &WindowChrome::toggleMaximized);
    connect(m_window, &QWindow::visibilityChanged, this, &WindowChrome::adoptVisibility);

    adoptVisibility();
    Q_EMIT metricsChanged();
}

bool WindowChrome::selectable() const
{
#if defined(HT_MUSIC_UNIFIED_TITLE_BAR) || defined(HT_MUSIC_NON_CLIENT_FRAME)
    return false;
#else
    return true;
#endif
}

bool WindowChrome::grips() const
{
#if defined(HT_MUSIC_NON_CLIENT_FRAME)
    return false;
#else
    return m_custom;
#endif
}

Qt::WindowFlags WindowChrome::windowFlags() const
{
    Qt::WindowFlags flags = Qt::Window;
#if !defined(HT_MUSIC_NON_CLIENT_FRAME)
    if (m_custom)
        flags |= Qt::FramelessWindowHint;
#endif
    return flags;
}

int WindowChrome::gripThickness() const
{
    return kGripThickness;
}

int WindowChrome::barHeight() const
{
    if (m_custom)
        return kBarHeight;
    if (m_frame && m_frame->titleAreaHeight() > 0)
        return m_frame->titleAreaHeight();
    return kNativeTitleArea;
}

int WindowChrome::leadingInset() const
{
    if (m_custom)
        return 0;
    if (m_frame && m_frame->leadingInset() > 0)
        return m_frame->leadingInset();
    return kNativeLeadingInset;
}

void WindowChrome::setDecorations(Decorations decorations)
{
    if (m_decorations == decorations)
        return;

    m_decorations = decorations;
    QSettings().setValue(kDecorationsKey, decorations == System ? kSystem : kCustom);
    Q_EMIT decorationsChanged();
}

void WindowChrome::setSnapRegion(const QRect &region)
{
    if (m_snapRegion == region)
        return;

    m_snapRegion = region;
    if (m_frame)
        m_frame->setSnapRegion(region);
    Q_EMIT snapRegionChanged();
}

void WindowChrome::startMove()
{
    if (m_window)
        m_window->startSystemMove();
}

void WindowChrome::startResize(int edges)
{
    if (m_window)
        m_window->startSystemResize(Qt::Edges(edges));
}

void WindowChrome::minimize()
{
    if (m_window)
        m_window->showMinimized();
}

void WindowChrome::toggleMaximized()
{
    if (!m_window)
        return;

    if (m_maximized)
        m_window->showNormal();
    else
        m_window->showMaximized();
}

void WindowChrome::close()
{
    if (m_window)
        m_window->close();
}

void WindowChrome::showSystemMenu()
{
    if (m_frame)
        m_frame->showSystemMenu();
}

void WindowChrome::relaunch()
{
    m_relaunching = true;
    QCoreApplication::quit();
}

void WindowChrome::adoptVisibility()
{
    const QWindow::Visibility visibility = m_window->visibility();
    const bool maximized = visibility == QWindow::Maximized || visibility == QWindow::FullScreen;
    if (m_maximized == maximized)
        return;

    m_maximized = maximized;
    qCDebug(logPlatform) << "window visibility" << visibility;
    Q_EMIT maximizedChanged();
}

}
