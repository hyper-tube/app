#pragma once

#include "ReleaseSection.h"

#include <QList>
#include <QString>

namespace update::releaseNotes {

QList<ReleaseSection> parse(const QString &markdown);

}
