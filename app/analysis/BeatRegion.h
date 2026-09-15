#pragma once

#include <QList>

namespace analysis {

class BeatRegion
{
public:
    qint64 startMs = 0;
    qint64 endMs = 0;
    QList<qint64> beatsMs;
    QList<qint64> downbeatsMs;
    double tempo = 0.0;
    double phaseMs = 0.0;
    double beatConfidence = 0.0;
    double downbeatConfidence = 0.0;

    bool valid() const;
    bool confident() const;
};

}
