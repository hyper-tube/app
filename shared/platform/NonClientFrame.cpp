#include "NonClientFrame.h"

#include <QCoreApplication>
#include <QWindow>

#include <dwmapi.h>
#include <shellapi.h>
#include <windowsx.h>

namespace {

constexpr UINT kNoEdge = UINT(-1);
constexpr UINT kDefaultDpi = 96;
constexpr DWORD kCornerPreference = 33;
constexpr DWORD kRoundedCorners = 2;

void roundCorners(HWND handle)
{
    const DWORD preference = kRoundedCorners;
    ::DwmSetWindowAttribute(handle, kCornerPreference, &preference, sizeof(preference));
}

HWND handleOf(QWindow *window)
{
    return window ? reinterpret_cast<HWND>(window->winId()) : nullptr;
}

UINT dpiOf(HWND handle)
{
    const UINT dpi = ::GetDpiForWindow(handle);
    return dpi > 0 ? dpi : kDefaultDpi;
}

int borderThickness(HWND handle)
{
    const UINT dpi = dpiOf(handle);
    return ::GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi)
        + ::GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
}

bool coversMonitor(HWND handle)
{
    RECT frame {};
    if (!::GetWindowRect(handle, &frame))
        return false;

    MONITORINFO monitor {};
    monitor.cbSize = sizeof(monitor);
    if (!::GetMonitorInfoW(::MonitorFromWindow(handle, MONITOR_DEFAULTTONEAREST), &monitor))
        return false;

    return ::EqualRect(&frame, &monitor.rcMonitor) != 0;
}

UINT autoHiddenBarEdge(HWND handle)
{
    APPBARDATA state {};
    state.cbSize = sizeof(state);
    if (!(::SHAppBarMessage(ABM_GETSTATE, &state) & ABS_AUTOHIDE))
        return kNoEdge;

    MONITORINFO monitor {};
    monitor.cbSize = sizeof(monitor);
    if (!::GetMonitorInfoW(::MonitorFromWindow(handle, MONITOR_DEFAULTTONEAREST), &monitor))
        return kNoEdge;

    for (const UINT edge : {ABE_BOTTOM, ABE_TOP, ABE_LEFT, ABE_RIGHT}) {
        APPBARDATA bar {};
        bar.cbSize = sizeof(bar);
        bar.uEdge = edge;
        bar.rc = monitor.rcMonitor;
        if (::SHAppBarMessage(ABM_GETAUTOHIDEBAREX, &bar))
            return edge;
    }
    return kNoEdge;
}

void enableMenuItem(HMENU menu, UINT item, bool enabled)
{
    MENUITEMINFOW info {};
    info.cbSize = sizeof(info);
    info.fMask = MIIM_STATE;
    info.fState = enabled ? MFS_ENABLED : MFS_DISABLED;
    ::SetMenuItemInfoW(menu, item, FALSE, &info);
}

}

namespace platform {

NonClientFrame::NonClientFrame(QWindow *window, QObject *parent)
    : WindowFrame(window, parent)
    , m_handle(handleOf(window))
{
    if (!m_handle)
        return;

    QCoreApplication::instance()->installNativeEventFilter(this);
    roundCorners(m_handle);
    ::SetWindowPos(m_handle, nullptr, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
}

NonClientFrame::~NonClientFrame()
{
    if (QCoreApplication *application = QCoreApplication::instance())
        application->removeNativeEventFilter(this);
}

void NonClientFrame::setSnapRegion(const QRect &region)
{
    m_snapRegion = region;
}

void NonClientFrame::showSystemMenu()
{
    HMENU menu = ::GetSystemMenu(m_handle, FALSE);
    if (!menu)
        return;

    const bool restored = ::IsZoomed(m_handle) == 0;
    enableMenuItem(menu, SC_RESTORE, !restored);
    enableMenuItem(menu, SC_MOVE, restored);
    enableMenuItem(menu, SC_SIZE, restored);
    enableMenuItem(menu, SC_MINIMIZE, true);
    enableMenuItem(menu, SC_MAXIMIZE, restored);
    enableMenuItem(menu, SC_CLOSE, true);
    ::SetMenuDefaultItem(menu, UINT(-1), FALSE);

    POINT cursor {};
    ::GetCursorPos(&cursor);
    const int command = ::TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, cursor.x, cursor.y,
                                         0, m_handle, nullptr);
    if (command)
        ::PostMessageW(m_handle, WM_SYSCOMMAND, WPARAM(command), 0);
}

bool NonClientFrame::nativeEventFilter(const QByteArray &type, void *message, qintptr *result)
{
    if (type != QByteArrayLiteral("windows_generic_MSG"))
        return false;

    const auto *msg = static_cast<MSG *>(message);
    if (msg->hwnd != m_handle)
        return false;

    qintptr discarded = 0;
    qintptr *answer = result ? result : &discarded;

    switch (msg->message) {
    case WM_NCCALCSIZE: return trimFrame(msg, answer);
    case WM_NCHITTEST: return hitTest(msg, answer);
    case WM_NCACTIVATE:
        *answer = ::DefWindowProcW(m_handle, WM_NCACTIVATE, msg->wParam, -1);
        return true;
    case WM_NCMOUSEMOVE:
    case WM_NCMOUSELEAVE:
    case WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONUP: return trackSnapButton(msg, answer);
    default: break;
    }
    return false;
}

bool NonClientFrame::trimFrame(const MSG *message, qintptr *result)
{
    if (!message->wParam)
        return false;

    RECT &client = reinterpret_cast<NCCALCSIZE_PARAMS *>(message->lParam)->rgrc[0];
    const bool zoomed = ::IsZoomed(m_handle) != 0;
    const bool fullScreen = coversMonitor(m_handle);

    if (zoomed && !fullScreen) {
        const int border = borderThickness(m_handle);
        client.left += border;
        client.top += border;
        client.right -= border;
        client.bottom -= border;
    }

    if (zoomed || fullScreen) {
        switch (autoHiddenBarEdge(m_handle)) {
        case ABE_TOP: client.top += 1; break;
        case ABE_BOTTOM: client.bottom -= 1; break;
        case ABE_LEFT: client.left += 1; break;
        case ABE_RIGHT: client.right -= 1; break;
        default: break;
        }
    }

    *result = 0;
    return true;
}

bool NonClientFrame::resizable() const
{
    const QWindow *target = window();
    if (!target)
        return true;
    const QSize smallest = target->minimumSize();
    const QSize largest = target->maximumSize();
    return smallest.width() != largest.width() || smallest.height() != largest.height();
}

bool NonClientFrame::hitTest(const MSG *message, qintptr *result)
{
    RECT frame {};
    if (!::GetWindowRect(m_handle, &frame))
        return false;

    const QPoint cursor(GET_X_LPARAM(message->lParam), GET_Y_LPARAM(message->lParam));

    const QWindow *target = window();
    if (target && !m_snapRegion.isEmpty()) {
        const qreal ratio = target->devicePixelRatio();
        const QRect region(frame.left + qRound(m_snapRegion.x() * ratio),
                           frame.top + qRound(m_snapRegion.y() * ratio),
                           qRound(m_snapRegion.width() * ratio),
                           qRound(m_snapRegion.height() * ratio));
        if (region.contains(cursor)) {
            *result = HTMAXBUTTON;
            return true;
        }
    }

    if (!::IsZoomed(m_handle) && resizable()) {
        const int border = borderThickness(m_handle);
        const bool left = cursor.x() < frame.left + border;
        const bool right = cursor.x() >= frame.right - border;
        const bool top = cursor.y() < frame.top + border;
        const bool bottom = cursor.y() >= frame.bottom - border;

        const qintptr edge = top ? (left        ? HTTOPLEFT
                                        : right ? HTTOPRIGHT
                                                : HTTOP)
            : bottom             ? (left        ? HTBOTTOMLEFT
                                        : right ? HTBOTTOMRIGHT
                                                : HTBOTTOM)
            : left               ? HTLEFT
            : right              ? HTRIGHT
                                 : HTCLIENT;
        if (edge != HTCLIENT) {
            *result = edge;
            return true;
        }
    }

    *result = HTCLIENT;
    return true;
}

bool NonClientFrame::trackSnapButton(const MSG *message, qintptr *result)
{
    const bool onButton = message->wParam == HTMAXBUTTON;

    switch (message->message) {
    case WM_NCMOUSEMOVE:
        if (!onButton)
            return false;
        if (!m_snapHovered) {
            m_snapHovered = true;
            TRACKMOUSEEVENT tracking {sizeof(TRACKMOUSEEVENT), TME_LEAVE | TME_NONCLIENT, m_handle,
                                      0};
            ::TrackMouseEvent(&tracking);
            Q_EMIT snapHoveredChanged(true);
        }
        *result = 0;
        return true;
    case WM_NCMOUSELEAVE:
        if (m_snapPressed) {
            m_snapPressed = false;
            Q_EMIT snapPressedChanged(false);
        }
        if (m_snapHovered) {
            m_snapHovered = false;
            Q_EMIT snapHoveredChanged(false);
        }
        return false;
    case WM_NCLBUTTONDOWN:
        if (!onButton)
            return false;
        m_snapPressed = true;
        Q_EMIT snapPressedChanged(true);
        *result = 0;
        return true;
    case WM_NCLBUTTONUP:
        if (!onButton)
            return false;
        if (m_snapPressed) {
            m_snapPressed = false;
            Q_EMIT snapPressedChanged(false);
            Q_EMIT snapTriggered();
        }
        *result = 0;
        return true;
    default: break;
    }
    return false;
}

}
