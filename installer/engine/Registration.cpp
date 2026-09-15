#include "Registration.h"

#include "Product.h"

#include <QDate>
#include <QDir>
#include <QFile>
#include <QSettings>

#include <windows.h>

#include <objbase.h>
#include <propkey.h>
#include <propvarutil.h>
#include <shlobj.h>
#include <shobjidl.h>

#include <string>

namespace {

QString native(const QString &path)
{
    return QDir::toNativeSeparators(path);
}

HKEY hiveOf(setup::Scope scope)
{
    return scope == setup::Scope::AllUsers ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
}

QString pathUnderHive(const QString &key)
{
    return key.section(QLatin1Char('\\'), 1);
}

const wchar_t *wide(const QString &text)
{
    return reinterpret_cast<const wchar_t *>(text.utf16());
}

QString readText(setup::Scope scope, const wchar_t *name)
{
    const QString key = pathUnderHive(setup::product::settingsKey(scope));
    DWORD size = 0;
    if (RegGetValueW(hiveOf(scope), wide(key), name, RRF_RT_REG_SZ, nullptr, nullptr, &size)
        != ERROR_SUCCESS) {
        return {};
    }

    std::wstring value(size / sizeof(wchar_t) + 1, L'\0');
    if (RegGetValueW(hiveOf(scope), wide(key), name, RRF_RT_REG_SZ, nullptr, value.data(), &size)
        != ERROR_SUCCESS) {
        return {};
    }
    return QString::fromWCharArray(value.data());
}

int readNumber(setup::Scope scope, const wchar_t *name)
{
    const QString key = pathUnderHive(setup::product::settingsKey(scope));
    DWORD value = 0;
    DWORD size = sizeof(value);
    if (RegGetValueW(hiveOf(scope), wide(key), name, RRF_RT_REG_DWORD, nullptr, &value, &size)
        != ERROR_SUCCESS) {
        return 0;
    }
    return int(value);
}

QString quoted(const QString &path)
{
    return QLatin1Char('"') + native(path) + QLatin1Char('"');
}

bool writeShortcut(const QString &linkPath, const QString &target, const QString &workingDir,
                   const QString &description, const QString &modelId)
{
    IShellLinkW *link = nullptr;
    if (CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW,
                         reinterpret_cast<void **>(&link))
        != S_OK) {
        return false;
    }

    bool ok = true;
    ok = ok && link->SetPath(reinterpret_cast<const wchar_t *>(native(target).utf16())) == S_OK;
    ok = ok
        && link->SetWorkingDirectory(reinterpret_cast<const wchar_t *>(native(workingDir).utf16()))
            == S_OK;
    ok = ok && link->SetDescription(reinterpret_cast<const wchar_t *>(description.utf16())) == S_OK;
    ok = ok
        && link->SetIconLocation(reinterpret_cast<const wchar_t *>(native(target).utf16()), 0)
            == S_OK;

    IPropertyStore *properties = nullptr;
    if (ok
        && link->QueryInterface(IID_IPropertyStore, reinterpret_cast<void **>(&properties))
            == S_OK) {
        PROPVARIANT value;
        if (InitPropVariantFromString(reinterpret_cast<const wchar_t *>(modelId.utf16()), &value)
            == S_OK) {
            properties->SetValue(PKEY_AppUserModel_ID, value);
            properties->Commit();
            PropVariantClear(&value);
        }
        properties->Release();
    }

    IPersistFile *file = nullptr;
    if (ok && link->QueryInterface(IID_IPersistFile, reinterpret_cast<void **>(&file)) == S_OK) {
        QDir().mkpath(QFileInfo(linkPath).absolutePath());
        ok = file->Save(reinterpret_cast<const wchar_t *>(native(linkPath).utf16()), TRUE) == S_OK;
        file->Release();
    } else {
        ok = false;
    }

    link->Release();
    return ok;
}

QString shortcutPath(const QString &folder)
{
    return QDir(folder).filePath(setup::product::displayName() + QStringLiteral(".lnk"));
}

}

namespace setup::registration {

bool write(const Options &options, const QString &directory, quint64 installedBytes,
           const QString &version)
{
    const QString installedVersion = version.isEmpty() ? product::version() : version;
    const QString target = QDir(directory).filePath(product::executable());
    const QString remover = QDir(directory).filePath(product::uninstaller());

    QSettings arp(product::uninstallKey(options.scope), QSettings::NativeFormat);
    arp.setValue(QStringLiteral("DisplayName"), product::displayName());
    arp.setValue(QStringLiteral("DisplayVersion"), installedVersion);
    arp.setValue(QStringLiteral("DisplayIcon"), native(target) + QStringLiteral(",0"));
    arp.setValue(QStringLiteral("Publisher"), product::publisher());
    arp.setValue(QStringLiteral("InstallLocation"), native(directory));
    arp.setValue(QStringLiteral("InstallDate"),
                 QDate::currentDate().toString(QStringLiteral("yyyyMMdd")));
    arp.setValue(QStringLiteral("EstimatedSize"), uint(installedBytes / 1024));
    arp.setValue(QStringLiteral("VersionMajor"),
                 installedVersion.section(QLatin1Char('.'), 0, 0).toInt());
    arp.setValue(QStringLiteral("VersionMinor"),
                 installedVersion.section(QLatin1Char('.'), 1, 1).toInt());
    arp.setValue(QStringLiteral("URLInfoAbout"), product::websiteUrl());
    arp.setValue(QStringLiteral("HelpLink"), product::repositoryUrl());
    arp.setValue(QStringLiteral("UninstallString"), quoted(remover));
    arp.setValue(QStringLiteral("QuietUninstallString"),
                 quoted(remover) + QStringLiteral(" /VERYSILENT"));
    arp.setValue(QStringLiteral("NoModify"), 1);
    arp.setValue(QStringLiteral("NoRepair"), 1);
    arp.sync();
    if (arp.status() != QSettings::NoError)
        return false;

    QSettings own(product::settingsKey(options.scope), QSettings::NativeFormat);
    own.setValue(QStringLiteral("InstallLocation"), native(directory));
    own.setValue(QStringLiteral("Version"), installedVersion);
    own.setValue(QStringLiteral("Scope"),
                 options.scope == Scope::AllUsers ? QStringLiteral("machine")
                                                  : QStringLiteral("user"));
    own.setValue(QStringLiteral("Components"), int(options.components.toInt()));
    own.setValue(QStringLiteral("Language"), options.language);
    own.sync();

    QSettings paths(product::appPathsKey(options.scope), QSettings::NativeFormat);
    paths.setValue(QStringLiteral("Default"), native(target));
    paths.setValue(QStringLiteral("Path"), native(directory));
    paths.sync();

    return own.status() == QSettings::NoError && paths.status() == QSettings::NoError;
}

void erase(Scope scope)
{
    QSettings(product::uninstallKey(scope), QSettings::NativeFormat).remove(QString());
    QSettings(product::settingsKey(scope), QSettings::NativeFormat).remove(QString());
    QSettings(product::appPathsKey(scope), QSettings::NativeFormat).remove(QString());
    QSettings(product::runKey(scope), QSettings::NativeFormat).remove(product::displayName());
    RegDeleteKeyW(hiveOf(scope), wide(pathUnderHive(product::settingsKey(scope))));
    RegDeleteKeyW(hiveOf(scope), wide(pathUnderHive(product::publisherKey(scope))));
}

Installed find(Scope scope)
{
    Installed found;
    const QString directory = readText(scope, L"InstallLocation");
    if (directory.isEmpty())
        return found;

    found.present = true;
    found.scope = scope;
    found.directory = QDir::fromNativeSeparators(directory);
    found.version = readText(scope, L"Version");
    found.components = Components::fromInt(readNumber(scope, L"Components"));
    found.language = readText(scope, L"Language");
    return found;
}

Installed find()
{
    const Installed machine = find(Scope::AllUsers);
    if (machine.present)
        return machine;
    return find(Scope::CurrentUser);
}

bool createShortcuts(const Options &options, const QString &directory)
{
    const QString target = QDir(directory).filePath(product::executable());
    bool ok = true;

    if (options.components.testFlag(Component::StartMenuShortcut)) {
        ok = writeShortcut(shortcutPath(product::startMenuDirectory(options.scope)), target,
                           directory, product::displayName(), product::appUserModelId())
            && ok;
    }
    if (options.components.testFlag(Component::DesktopShortcut)) {
        ok = writeShortcut(shortcutPath(product::desktopDirectory(options.scope)), target,
                           directory, product::displayName(), product::appUserModelId())
            && ok;
    }
    return ok;
}

void removeShortcuts(Scope scope)
{
    QFile::remove(shortcutPath(product::startMenuDirectory(scope)));
    QFile::remove(shortcutPath(product::desktopDirectory(scope)));
}

bool setLaunchAtSignIn(bool enabled, Scope scope, const QString &directory)
{
    QSettings run(product::runKey(scope), QSettings::NativeFormat);
    if (!enabled) {
        run.remove(product::displayName());
        run.sync();
        return run.status() == QSettings::NoError;
    }
    run.setValue(product::displayName(), quoted(QDir(directory).filePath(product::executable())));
    run.sync();
    return run.status() == QSettings::NoError;
}

bool registerUrlScheme(Scope scope, const QString &directory)
{
    const QString root = (scope == Scope::AllUsers ? QStringLiteral("HKEY_LOCAL_MACHINE")
                                                   : QStringLiteral("HKEY_CURRENT_USER"))
        + QStringLiteral("\\Software\\Classes\\") + product::urlScheme();
    const QString target = QDir(directory).filePath(product::executable());

    QSettings scheme(root, QSettings::NativeFormat);
    scheme.setValue(QStringLiteral("Default"), QStringLiteral("URL:") + product::displayName());
    scheme.setValue(QStringLiteral("URL Protocol"), QString());
    scheme.setValue(QStringLiteral("DefaultIcon/Default"), native(target) + QStringLiteral(",0"));
    scheme.setValue(QStringLiteral("shell/open/command/Default"),
                    quoted(target) + QStringLiteral(" \"%1\""));
    scheme.sync();
    return scheme.status() == QSettings::NoError;
}

void unregisterUrlScheme(Scope scope)
{
    const QString root = (scope == Scope::AllUsers ? QStringLiteral("HKEY_LOCAL_MACHINE")
                                                   : QStringLiteral("HKEY_CURRENT_USER"))
        + QStringLiteral("\\Software\\Classes\\") + product::urlScheme();
    QSettings(root, QSettings::NativeFormat).remove(QString());
}

}
