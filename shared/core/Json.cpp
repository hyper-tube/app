#include "Json.h"

#include <QJsonArray>
#include <QStringList>

namespace core::json {

qint64 toInt(const QJsonValue &value, qint64 fallback)
{
    if (value.isDouble())
        return static_cast<qint64>(value.toDouble());

    if (value.isString()) {
        bool parsed = false;
        const qint64 number = value.toString().toLongLong(&parsed);
        if (parsed)
            return number;
    }
    return fallback;
}

double toDouble(const QJsonValue &value, double fallback)
{
    if (value.isDouble())
        return value.toDouble();

    if (value.isString()) {
        bool parsed = false;
        const double number = value.toString().toDouble(&parsed);
        if (parsed)
            return number;
    }
    return fallback;
}

QJsonValue at(const QJsonValue &root, const QStringList &path)
{
    QJsonValue value = root;
    for (const QString &key : path) {
        if (!value.isObject())
            return {};
        value = value.toObject().value(key);
    }
    return value;
}

QJsonObject object(const QJsonValue &root, const QStringList &path)
{
    return at(root, path).toObject();
}

}
