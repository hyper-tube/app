#include "Product.h"

#include "BuildInfo.h"

#include <QDir>
#include <QStandardPaths>

#include <windows.h>

#include <shlobj.h>

namespace {

QString knownFolder(const KNOWNFOLDERID &id)
{
    PWSTR raw = nullptr;
    if (SHGetKnownFolderPath(id, KF_FLAG_DONT_VERIFY, nullptr, &raw) != S_OK)
        return {};
    const QString path = QString::fromWCharArray(raw);
    CoTaskMemFree(raw);
    return QDir::fromNativeSeparators(path);
}

QString latin(const char *value)
{
    return QString::fromLatin1(value);
}

}

namespace setup::product {

QString displayName()
{
    return latin(core::build::kDisplayName);
}

QString version()
{
    return latin(core::build::kVersion);
}

QString publisher()
{
    return latin(core::build::kPublisher);
}

QString executable()
{
    return latin(core::build::kIdentifier) + QStringLiteral(".exe");
}

QString uninstaller()
{
    return QStringLiteral("Uninstall.exe");
}

QString folderName()
{
    return latin(core::build::kProductFolder);
}

QString appUserModelId()
{
    return latin(core::build::kAppUserModelId);
}

QString websiteUrl()
{
    return latin(core::build::kWebsiteUrl);
}

QString repositoryUrl()
{
    return latin(core::build::kRepositoryUrl);
}

QString urlScheme()
{
    return latin(core::build::kUrlScheme);
}

QString defaultDirectory(Scope scope)
{
    const QString root = scope == Scope::AllUsers ? knownFolder(FOLDERID_ProgramFiles)
                                                  : knownFolder(FOLDERID_UserProgramFiles);
    return QDir(root).filePath(folderName());
}

QString startMenuDirectory(Scope scope)
{
    return scope == Scope::AllUsers ? knownFolder(FOLDERID_CommonPrograms)
                                    : knownFolder(FOLDERID_Programs);
}

QString desktopDirectory(Scope scope)
{
    return scope == Scope::AllUsers ? knownFolder(FOLDERID_PublicDesktop)
                                    : knownFolder(FOLDERID_Desktop);
}

QString uninstallKey(Scope scope)
{
    return (scope == Scope::AllUsers ? QStringLiteral("HKEY_LOCAL_MACHINE")
                                     : QStringLiteral("HKEY_CURRENT_USER"))
        + QStringLiteral("\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\")
        + latin(core::build::kUninstallKey);
}

QString publisherKey(Scope scope)
{
    return (scope == Scope::AllUsers ? QStringLiteral("HKEY_LOCAL_MACHINE")
                                     : QStringLiteral("HKEY_CURRENT_USER"))
        + QStringLiteral("\\Software\\") + publisher();
}

QString settingsKey(Scope scope)
{
    return publisherKey(scope) + QLatin1Char('\\') + folderName();
}

QString appPathsKey(Scope scope)
{
    return (scope == Scope::AllUsers ? QStringLiteral("HKEY_LOCAL_MACHINE")
                                     : QStringLiteral("HKEY_CURRENT_USER"))
        + QStringLiteral("\\Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\")
        + executable();
}

QString runKey(Scope scope)
{
    return (scope == Scope::AllUsers ? QStringLiteral("HKEY_LOCAL_MACHINE")
                                     : QStringLiteral("HKEY_CURRENT_USER"))
        + QStringLiteral("\\Software\\Microsoft\\Windows\\CurrentVersion\\Run");
}

QString userConfigDirectory()
{
    return QDir(knownFolder(FOLDERID_RoamingAppData)).filePath(latin(core::build::kIdentifier));
}

QString userDataDirectory()
{
    return QDir(knownFolder(FOLDERID_LocalAppData)).filePath(latin(core::build::kIdentifier));
}

}
