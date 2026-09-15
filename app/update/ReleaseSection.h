#pragma once

#include "ReleaseNote.h"

#include <QList>
#include <QMetaType>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace update {

class ReleaseSection
{
    Q_GADGET
    QML_VALUE_TYPE(releaseSection)

    Q_PROPERTY(QString title MEMBER title)
    Q_PROPERTY(QString icon MEMBER icon)
    Q_PROPERTY(bool scoped MEMBER scoped)
    Q_PROPERTY(QList<update::ReleaseNote> notes MEMBER notes)

public:
    QString title;
    QString icon;
    QList<ReleaseNote> notes;
    bool scoped = false;

    bool operator==(const ReleaseSection &) const = default;
};

}

Q_DECLARE_METATYPE(update::ReleaseSection)
