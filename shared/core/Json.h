#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QString>

namespace core::json {

qint64 toInt(const QJsonValue &value, qint64 fallback = 0);
double toDouble(const QJsonValue &value, double fallback = 0.0);
QJsonValue at(const QJsonValue &root, const QStringList &path);
QJsonObject object(const QJsonValue &root, const QStringList &path);

}
