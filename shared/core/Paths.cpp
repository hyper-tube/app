#include "Paths.h"

#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

namespace {

QString ensure(const QString &path)
{
    QDir().mkpath(path);
    return path;
}

}

namespace core::paths {

QString configDir()
{
    return ensure(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
}

QString dataDir()
{
    return ensure(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
}

QString cacheDir()
{
    return ensure(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
}

QString artworkCacheDir()
{
    return ensure(cacheDir() + QStringLiteral("/artwork"));
}

QString logDir()
{
#ifdef Q_OS_MACOS
    const QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    return ensure(home + QStringLiteral("/Library/Logs/") + QCoreApplication::applicationName());
#else
    const QString state = QStandardPaths::writableLocation(QStandardPaths::StateLocation);
    return ensure(state + QStringLiteral("/logs"));
#endif
}

QString crashReportDir()
{
#ifdef Q_OS_MACOS
    const QString root = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
#else
    const QString root = QStandardPaths::writableLocation(QStandardPaths::StateLocation);
#endif
    return ensure(root + QStringLiteral("/crash-reports"));
}

}
