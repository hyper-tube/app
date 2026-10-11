#pragma once

#include <QString>
#include <QVariant>

namespace theme {

struct Token
{
    enum Kind {
        Step,
        Radius,
        Real,
        Duration,
        Curve,
        Family,
    };

    QString name;
    Kind kind = Real;
    QVariant fallback;
    qreal minimum = 0;
    qreal maximum = 0;
    QString description;
};

}
