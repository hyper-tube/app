#include "UnifiedTitleBar.h"

#include <QWindow>

#import <AppKit/AppKit.h>

#include <cmath>

namespace {

NSWindow *nativeWindowOf(QWindow *window)
{
    if (!window)
        return nil;

    auto *view = reinterpret_cast<NSView *>(window->winId());
    return view ? [view window] : nil;
}

}

namespace platform {

UnifiedTitleBar::UnifiedTitleBar(QWindow *window, QObject *parent)
    : WindowFrame(window, parent)
{
    NSWindow *native = nativeWindowOf(window);
    if (!native)
        return;

    const NSRect frame = native.frame;
    const NSRect content = [NSWindow contentRectForFrameRect:frame styleMask:native.styleMask];
    m_titleAreaHeight = int(std::lround(NSHeight(frame) - NSHeight(content)));

    native.titlebarAppearsTransparent = YES;
    native.titleVisibility = NSWindowTitleHidden;
    native.styleMask |= NSWindowStyleMaskFullSizeContentView;
    native.movableByWindowBackground = NO;

    NSButton *close = [native standardWindowButton:NSWindowCloseButton];
    NSButton *zoom = [native standardWindowButton:NSWindowZoomButton];
    if (close && zoom)
        m_leadingInset = int(std::lround(NSMaxX(zoom.frame) + NSMinX(close.frame)));
}

}
