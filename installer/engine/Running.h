#pragma once

#include <QString>
#include <QStringList>

namespace setup::running {

QStringList inside(const QString &directory);

bool askToClose(const QString &directory, int millisecondsToWait);

}
