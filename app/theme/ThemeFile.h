#pragma once

#include "ThemeScheme.h"
#include "Token.h"

#include <QColor>
#include <QCoreApplication>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QStringList>
#include <QVariantHash>

namespace theme {

class ThemeFile
{
    Q_DECLARE_TR_FUNCTIONS(theme::ThemeFile)

public:
    static ThemeFile read(const QString &id, const QString &path);
    static int format();

    QString directory() const;

    QString id;
    QString path;
    QString name;
    QString description;
    QString author;
    QString extends;
    QVariantHash values;
    QColor seed;
    ThemeScheme::Roles light;
    ThemeScheme::Roles dark;
    ThemeScheme::Roles lightOverrides;
    ThemeScheme::Roles darkOverrides;
    QStringList fonts;
    QStringList problems;

private:
    static QString expectation(const Token &token);

    void take(const QJsonObject &document);
    void collect(const QString &name, const QJsonValue &value);
    void takeColors(const QJsonValue &value);
    void takeOverrides(const QJsonValue &value);
    void takeSchemes(const QJsonValue &value);
    void takeFonts(const QJsonValue &value);
    ThemeScheme::Roles rolesFrom(const QString &where, const QJsonValue &value);
    QString textFrom(const QString &where, const QJsonValue &value);
};

}
