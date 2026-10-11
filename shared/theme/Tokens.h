#pragma once

#include "Token.h"

#include <QJsonValue>
#include <QList>
#include <QStringList>
#include <QVariantHash>
#include <QVariantMap>

#include <optional>

namespace theme::tokens {

const QList<Token> &all();
const Token *find(const QString &name);
const QStringList &steps();

std::optional<QVariant> accept(const Token &token, const QJsonValue &value);
QVariantMap resolve(const QVariantHash &chosen);
const QVariantMap &defaults();

}
