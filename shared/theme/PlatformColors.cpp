#include "PlatformColors.h"

#if defined(Q_OS_WIN)
#include <QAbstractNativeEventFilter>
#include <QGuiApplication>
#include <QSettings>

#include <windows.h>

#include <functional>
#elif defined(Q_OS_MACOS)
#include "AppleColors.h"
#elif defined(Q_OS_UNIX)
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusVariant>
#endif

namespace {

#if defined(Q_OS_WIN)

constexpr quint32 kOpaqueMask = 0x00FFFFFF;

const QString kDwmKey = QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\DWM");
const QString kPersonalizeKey = QStringLiteral(
    "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize");
const QString kAccentValue = QStringLiteral("AccentColor");
const QString kColorizationValue = QStringLiteral("ColorizationColor");
const QString kLightThemeValue = QStringLiteral("AppsUseLightTheme");

QString sourceName()
{
    return QStringLiteral("Windows accent color");
}

QString sourceLocation()
{
    return kDwmKey;
}

QColor readAccent()
{
    const QSettings dwm(kDwmKey, QSettings::NativeFormat);

    const QVariant accent = dwm.value(kAccentValue);
    if (accent.isValid()) {
        const quint32 abgr = accent.toUInt() & kOpaqueMask;
        return QColor::fromRgb(abgr & 0xFF, (abgr >> 8) & 0xFF, (abgr >> 16) & 0xFF);
    }

    const QVariant colorization = dwm.value(kColorizationValue);
    if (colorization.isValid())
        return QColor::fromRgb(colorization.toUInt() & kOpaqueMask);

    return {};
}

Qt::ColorScheme readColorScheme()
{
    const QSettings personalize(kPersonalizeKey, QSettings::NativeFormat);
    const QVariant light = personalize.value(kLightThemeValue);
    if (!light.isValid())
        return Qt::ColorScheme::Unknown;
    return light.toUInt() == 0 ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light;
}

class ColorizationWatcher : public QObject, public QAbstractNativeEventFilter
{
public:
    ColorizationWatcher(QObject *parent, std::function<void()> notify)
        : QObject(parent)
        , m_notify(std::move(notify))
    {
        qApp->installNativeEventFilter(this);
    }

    ~ColorizationWatcher() override { qApp->removeNativeEventFilter(this); }

    bool nativeEventFilter(const QByteArray &type, void *message, qintptr *) override
    {
        if (type != "windows_generic_MSG")
            return false;
        const MSG *event = static_cast<MSG *>(message);
        if (event->message == WM_DWMCOLORIZATIONCOLORCHANGED
            || event->message == WM_SETTINGCHANGE) {
            m_notify();
        }
        return false;
    }

private:
    std::function<void()> m_notify;
};

#elif defined(Q_OS_MACOS)

QString sourceName()
{
    return QStringLiteral("macOS accent color");
}

QString sourceLocation()
{
    return QStringLiteral("System Settings > Appearance");
}

QColor readAccent()
{
    return theme::apple::accentColor();
}

Qt::ColorScheme readColorScheme()
{
    return theme::apple::colorScheme();
}

#elif defined(Q_OS_UNIX)

constexpr int kPortalTimeoutMs = 500;
constexpr uint kPreferDark = 1;
constexpr uint kPreferLight = 2;

const QString kPortalService = QStringLiteral("org.freedesktop.portal.Desktop");
const QString kPortalPath = QStringLiteral("/org/freedesktop/portal/desktop");
const QString kSettingsInterface = QStringLiteral("org.freedesktop.portal.Settings");
const QString kAppearanceNamespace = QStringLiteral("org.freedesktop.appearance");
const QString kAccentKey = QStringLiteral("accent-color");
const QString kColorSchemeKey = QStringLiteral("color-scheme");

QString sourceName()
{
    return QStringLiteral("Desktop portal");
}

QString sourceLocation()
{
    return kAppearanceNamespace;
}

QVariant portalRead(const QString &key)
{
    QDBusInterface settings(kPortalService, kPortalPath, kSettingsInterface,
                            QDBusConnection::sessionBus());
    if (!settings.isValid())
        return {};
    settings.setTimeout(kPortalTimeoutMs);

    const QDBusReply<QDBusVariant> one =
        settings.call(QStringLiteral("ReadOne"), kAppearanceNamespace, key);
    if (one.isValid())
        return one.value().variant();

    const QDBusReply<QDBusVariant> legacy =
        settings.call(QStringLiteral("Read"), kAppearanceNamespace, key);
    if (!legacy.isValid())
        return {};

    const QVariant wrapped = legacy.value().variant();
    return wrapped.canConvert<QDBusVariant>() ? wrapped.value<QDBusVariant>().variant() : wrapped;
}

QColor readAccent()
{
    const QVariant value = portalRead(kAccentKey);
    if (!value.canConvert<QDBusArgument>())
        return {};

    QDBusArgument argument = value.value<QDBusArgument>();
    double red = -1;
    double green = -1;
    double blue = -1;
    argument.beginStructure();
    argument >> red >> green >> blue;
    argument.endStructure();

    if (red < 0 || green < 0 || blue < 0)
        return {};
    return QColor::fromRgbF(qBound(0.0, red, 1.0), qBound(0.0, green, 1.0), qBound(0.0, blue, 1.0));
}

Qt::ColorScheme readColorScheme()
{
    const QVariant value = portalRead(kColorSchemeKey);
    if (!value.isValid())
        return Qt::ColorScheme::Unknown;

    switch (value.toUInt()) {
    case kPreferDark: return Qt::ColorScheme::Dark;
    case kPreferLight: return Qt::ColorScheme::Light;
    default: break;
    }
    return Qt::ColorScheme::Unknown;
}

#else

QString sourceName()
{
    return {};
}

QString sourceLocation()
{
    return {};
}

QColor readAccent()
{
    return {};
}

Qt::ColorScheme readColorScheme()
{
    return Qt::ColorScheme::Unknown;
}

#endif

}

namespace theme {

PlatformColors::PlatformColors(QObject *parent)
    : ColorSource(QStringLiteral("platform"), sourceName(), sourceLocation(), parent)
{
    observe();
}

void PlatformColors::refresh()
{
    adopt({}, readAccent(), readColorScheme());
}

void PlatformColors::observe()
{
#if defined(Q_OS_WIN)
    new ColorizationWatcher(this, [this] { refresh(); });
#elif defined(Q_OS_MACOS)
    apple::observeAppearance(this, [this] { refresh(); });
#elif defined(Q_OS_UNIX)
    QDBusConnection::sessionBus().connect(kPortalService, kPortalPath, kSettingsInterface,
                                          QStringLiteral("SettingChanged"), this, SLOT(refresh()));
#endif
}

}
