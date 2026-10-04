#pragma once

#include <QMetaType>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace media {

class Credit
{
    Q_GADGET
    QML_VALUE_TYPE(credit)

    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString browseId MEMBER browseId)
    Q_PROPERTY(QString kind MEMBER kind)

public:
    QString name;
    QString browseId;
    QString kind;

    bool operator==(const Credit &) const = default;
};

}

Q_DECLARE_METATYPE(media::Credit)
