#pragma once

#include <QMetaType>
#include <QString>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

namespace update {

class ReleaseNote
{
    Q_GADGET
    QML_VALUE_TYPE(releaseNote)

    Q_PROPERTY(QString scope MEMBER scope)
    Q_PROPERTY(QString text MEMBER text)
    Q_PROPERTY(QString commit MEMBER commit)
    Q_PROPERTY(QUrl commitUrl MEMBER commitUrl)
    Q_PROPERTY(bool breaking MEMBER breaking)

public:
    QString scope;
    QString text;
    QString commit;
    QUrl commitUrl;
    bool breaking = false;

    bool operator==(const ReleaseNote &) const = default;
};

}

Q_DECLARE_METATYPE(update::ReleaseNote)
