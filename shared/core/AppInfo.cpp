#include "AppInfo.h"

#include "BuildInfo.h"
#include "Logging.h"

#include <QGuiApplication>

#ifdef Q_OS_WIN
#include <shobjidl.h>
#endif

namespace core {

AppInfo::AppInfo(QObject *parent)
    : QObject(parent)
{
}

void AppInfo::identify()
{
    QCoreApplication::setApplicationName(identifier());
    QCoreApplication::setOrganizationName(identifier());
    QCoreApplication::setOrganizationDomain(QString::fromLatin1(build::kOrganizationDomain));
    QCoreApplication::setApplicationVersion(version());
    QGuiApplication::setApplicationDisplayName(name());
    QGuiApplication::setDesktopFileName(identifier());

#ifdef Q_OS_WIN
    const QString modelId = QString::fromLatin1(build::kAppUserModelId);
    SetCurrentProcessExplicitAppUserModelID(reinterpret_cast<const wchar_t *>(modelId.utf16()));
#endif
}

QString AppInfo::name()
{
    return QString::fromLatin1(build::kDisplayName);
}

QString AppInfo::identifier()
{
    return QString::fromLatin1(build::kIdentifier);
}

QString AppInfo::version()
{
    return QString::fromLatin1(build::kVersion);
}

QString AppInfo::website()
{
    return QString::fromLatin1(build::kWebsiteUrl);
}

QString AppInfo::repository()
{
    return QString::fromLatin1(build::kRepositoryUrl);
}

QString AppInfo::logFile()
{
    return logging::file();
}

QUrl AppInfo::logFolder()
{
    return QUrl::fromLocalFile(logging::folder());
}

}
