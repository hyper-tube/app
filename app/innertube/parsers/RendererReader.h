#pragma once

#include "model/Item.h"

#include <QJsonObject>
#include <QJsonValue>

namespace innertube::parsers {

QJsonValue findFirst(const QJsonValue &root, const QString &key);
bool containsText(const QJsonValue &root, const QString &key, const QString &value);
QString readText(const QJsonValue &node);
QString readThumbnail(const QJsonValue &node);
QString readContinuation(const QJsonObject &container);
model::Item readItem(const QJsonObject &renderer);
model::Item readHeader(const QJsonObject &root);

}
