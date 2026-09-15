#pragma once

#include <QMetaType>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace model {

class SortOption
{
    Q_GADGET
    QML_VALUE_TYPE(sortOption)

    Q_PROPERTY(QString title MEMBER title)
    Q_PROPERTY(bool selected MEMBER selected)
    Q_PROPERTY(bool stored READ stored)

public:
    QString title;
    QString params;
    QString continuation;
    QString formValue;
    int videoOrder = -1;
    int dynamicSort = -1;
    bool selected = false;

    bool stored() const { return videoOrder >= 0 || dynamicSort >= 0; }
    bool valid() const
    {
        return stored() || !params.isEmpty() || !continuation.isEmpty() || !formValue.isEmpty();
    }

    bool operator==(const SortOption &) const = default;
};

}

Q_DECLARE_METATYPE(model::SortOption)
