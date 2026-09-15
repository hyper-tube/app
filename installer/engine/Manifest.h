#pragma once

#include "Options.h"

#include <QString>
#include <QMap>
#include <QStringList>

namespace setup {

struct Manifest
{
    bool valid = false;
    QString version;
    QString language;
    Scope scope = Scope::CurrentUser;
    Components components;
    QStringList files;
    QMap<QString, quint64> sizes;
    QMap<QString, QString> hashes;

    static QString pathIn(const QString &directory);
    static Manifest read(const QString &directory);
    bool write(const QString &directory) const;
};

}
