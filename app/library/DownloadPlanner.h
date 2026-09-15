#pragma once

#include "PlayLog.h"

#include <QSet>

namespace library {

class DownloadPlanner
{
public:
    static QList<media::Track> rank(const QHash<QString, PlayLog::Entry> &entries);
    static QSet<QString> choose(const QList<media::Track> &ranked, int count, qint64 budget,
                                const QHash<QString, qint64> &sizes, const QSet<QString> &forced);
};

}
