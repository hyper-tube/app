#pragma once

#include <QMetaType>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace theme {

class ColorSourceInfo
{
    Q_GADGET
    QML_VALUE_TYPE(colorSourceInfo)

    Q_PROPERTY(QString id MEMBER id)
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString location MEMBER location)
    Q_PROPERTY(bool active MEMBER active)
    Q_PROPERTY(bool complete MEMBER complete)

public:
    QString id;
    QString name;
    QString location;
    bool active = false;
    bool complete = false;

    bool operator==(const ColorSourceInfo &) const = default;
};

}

Q_DECLARE_METATYPE(theme::ColorSourceInfo)
