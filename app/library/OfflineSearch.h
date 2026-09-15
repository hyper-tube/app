#pragma once

#include "media/Track.h"

#include <QList>
#include <QString>
#include <QStringList>

namespace library::offlineSearch {

QList<media::Track> matches(const QString &query);
QStringList suggestions(const QString &query, int limit);

}
