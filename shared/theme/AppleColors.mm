#include "AppleColors.h"

#include <QObject>

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

namespace {

NSString *const kInterfaceStyleKey = @"AppleInterfaceStyle";
NSString *const kDarkInterfaceStyle = @"Dark";
NSString *const kThemeChangedNotification = @"AppleInterfaceThemeChangedNotification";
NSString *const kColorsChangedNotification = @"AppleColorPreferencesChangedNotification";

id observe(NSString *name, const std::function<void()> &notify)
{
    return [[NSDistributedNotificationCenter defaultCenter]
        addObserverForName:name
                    object:nil
                     queue:[NSOperationQueue mainQueue]
                usingBlock:^(NSNotification *) { notify(); }];
}

}

namespace theme::apple {

QColor accentColor()
{
    @autoreleasepool {
        NSColor *accent =
            [[NSColor controlAccentColor] colorUsingColorSpace:[NSColorSpace sRGBColorSpace]];
        if (!accent)
            return {};
        return QColor::fromRgbF(accent.redComponent, accent.greenComponent, accent.blueComponent);
    }
}

Qt::ColorScheme colorScheme()
{
    @autoreleasepool {
        NSString *style = [[NSUserDefaults standardUserDefaults] stringForKey:kInterfaceStyleKey];
        const bool dark = style != nil &&
            [style rangeOfString:kDarkInterfaceStyle options:NSCaseInsensitiveSearch].location
                != NSNotFound;
        return dark ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light;
    }
}

void observeAppearance(QObject *owner, std::function<void()> notify)
{
    NSArray *tokens = @[
        observe(kThemeChangedNotification, notify), observe(kColorsChangedNotification, notify)
    ];
    [tokens retain];

    QObject::connect(owner, &QObject::destroyed, owner, [tokens] {
        for (id token in tokens)
            [[NSDistributedNotificationCenter defaultCenter] removeObserver:token];
        [tokens release];
    });
}

}
