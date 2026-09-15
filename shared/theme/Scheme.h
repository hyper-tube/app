#pragma once

#include <QColor>
#include <QHash>
#include <QString>

namespace theme {

QHash<QString, QColor> schemeFrom(const QColor &seed, bool dark);

}
