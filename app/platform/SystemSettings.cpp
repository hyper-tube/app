#include "SystemSettings.h"

#include "core/AppInfo.h"

#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QSystemTrayIcon>

namespace {

constexpr auto kCloseToTrayKey = "system/closeToTray";

#ifdef Q_OS_WIN
constexpr auto kRunKey = "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";

QString launcherCommand()
{
    return QLatin1Char('"') + QDir::toNativeSeparators(QCoreApplication::applicationFilePath())
        + QLatin1Char('"');
}
#endif

}

namespace platform {

SystemSettings::SystemSettings(QObject *parent)
    : QObject(parent)
{
    const QSettings settings;
    m_closeToTray = settings.value(kCloseToTrayKey, true).toBool();

#ifdef Q_OS_WIN
    const QSettings run(QLatin1String(kRunKey), QSettings::NativeFormat);
    m_launchAtSignIn = !run.value(core::AppInfo::name()).toString().isEmpty();
#endif
}

SystemSettings &SystemSettings::instance()
{
    static auto *settings = new SystemSettings(QCoreApplication::instance());
    return *settings;
}

SystemSettings *SystemSettings::create(QQmlEngine *, QJSEngine *)
{
    QQmlEngine::setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
    return &instance();
}

bool SystemSettings::available() const
{
    return background() != Unavailable || startupAvailable();
}

SystemSettings::Background SystemSettings::background() const
{
#if defined(Q_OS_WIN)
    return QSystemTrayIcon::isSystemTrayAvailable() ? Tray : Unavailable;
#elif defined(Q_OS_MACOS)
    return Dock;
#else
    return Unavailable;
#endif
}

bool SystemSettings::startupAvailable() const
{
#ifdef Q_OS_WIN
    return true;
#else
    return false;
#endif
}

void SystemSettings::setCloseToTray(bool keep)
{
    if (m_closeToTray == keep)
        return;
    m_closeToTray = keep;
    QSettings().setValue(kCloseToTrayKey, keep);
    Q_EMIT changed();
}

void SystemSettings::setLaunchAtSignIn(bool launch)
{
    if (m_launchAtSignIn == launch)
        return;
    m_launchAtSignIn = launch;

#ifdef Q_OS_WIN
    QSettings run(QLatin1String(kRunKey), QSettings::NativeFormat);
    if (launch)
        run.setValue(core::AppInfo::name(), launcherCommand());
    else
        run.remove(core::AppInfo::name());
#endif

    Q_EMIT changed();
}

}
