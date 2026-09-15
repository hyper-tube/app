#include "WindowFrame.h"

#if defined(HT_MUSIC_NON_CLIENT_FRAME)
#include "NonClientFrame.h"
#elif defined(HT_MUSIC_UNIFIED_TITLE_BAR)
#include "UnifiedTitleBar.h"
#endif

namespace platform {

WindowFrame::WindowFrame(QWindow *window, QObject *parent)
    : QObject(parent)
    , m_window(window)
{
}

WindowFrame *WindowFrame::create(QWindow *window, QObject *parent)
{
#if defined(HT_MUSIC_NON_CLIENT_FRAME)
    return new NonClientFrame(window, parent);
#elif defined(HT_MUSIC_UNIFIED_TITLE_BAR)
    return new UnifiedTitleBar(window, parent);
#else
    return new WindowFrame(window, parent);
#endif
}

void WindowFrame::setSnapRegion(const QRect &) { }

void WindowFrame::showSystemMenu() { }

}
