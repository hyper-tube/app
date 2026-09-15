#pragma once

#include "Value.h"

#include <QByteArray>
#include <QList>
#include <QString>

#include <span>

namespace diagnostics::backend {

struct Configuration
{
    QString dsn;
    QString release;
    QString environment;
    QString databasePath;
    QString handlerPath;
    bool verbose = false;
};

struct Event
{
    QByteArray type;
    QByteArray description;
    QByteArray logger;
    Level level = Level::Error;
    QList<Field> fields;
    QList<QByteArray> fingerprint;
    bool stacktrace = true;
};

enum class Ending {
    Shutdown,
    Revoked,
};

bool compiled();
bool start(const Configuration &configuration);
void stop(Ending ending);
bool running();

void addBreadcrumb(const char *category, const QByteArray &message, std::span<const Field> fields,
                   Level level);
void capture(const Event &event);
void setTag(const char *key, const QByteArray &value);
void setContext(const char *name, std::span<const Field> fields);
void crash();

}
