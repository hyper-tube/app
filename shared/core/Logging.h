#pragma once

#include <QLoggingCategory>
#include <QString>
#include <QStringView>

Q_DECLARE_LOGGING_CATEGORY(logTheme)
Q_DECLARE_LOGGING_CATEGORY(logPlayback)
Q_DECLARE_LOGGING_CATEGORY(logArtwork)
Q_DECLARE_LOGGING_CATEGORY(logNet)
Q_DECLARE_LOGGING_CATEGORY(logInnerTube)
Q_DECLARE_LOGGING_CATEGORY(logStream)
Q_DECLARE_LOGGING_CATEGORY(logTransition)
Q_DECLARE_LOGGING_CATEGORY(logPlatform)
Q_DECLARE_LOGGING_CATEGORY(logPlugins)
Q_DECLARE_LOGGING_CATEGORY(logDiagnostics)

namespace core::logging {

struct Record
{
    QtMsgType type = QtDebugMsg;
    QLatin1StringView category;
    QLatin1StringView file;
    int line = 0;
    QString function;
    QString thread;
    QStringView message;
};

using Observer = void (*)(const Record &record);

void install(const QString &path = {});
void observe(Observer observer);

QLatin1StringView fileName(const char *path);
QString functionName(const char *signature);

QString folder();
QString file();

}
