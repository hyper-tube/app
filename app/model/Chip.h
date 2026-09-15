#pragma once

#include <QMetaType>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace model {

class Chip
{
    Q_GADGET
    QML_VALUE_TYPE(chip)

    Q_PROPERTY(QString title MEMBER title)
    Q_PROPERTY(QString icon MEMBER icon)
    Q_PROPERTY(QString browseId MEMBER browseId)
    Q_PROPERTY(QString params MEMBER params)
    Q_PROPERTY(bool selected MEMBER selected)
    Q_PROPERTY(bool clearing READ clearing)

public:
    QString title;
    QString icon;
    QString browseId;
    QString params;
    QString continuation;
    QString deselection;
    bool selected = false;

    bool clearing() const { return title.isEmpty(); }
    bool reloads() const { return !continuation.isEmpty(); }
    bool operator==(const Chip &) const = default;
};

}

Q_DECLARE_METATYPE(model::Chip)
