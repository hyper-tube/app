#pragma once

#include "BeatRegion.h"
#include "KeyEstimator.h"
#include "Structure.h"

#include <QList>
#include <QString>

namespace analysis {

class TrackAnalysis
{
public:
    static constexpr int kVersion = 8;

    int version = kVersion;
    QString videoId;
    int itag = 0;
    qint64 durationMs = 0;
    qint64 leadingSilenceEndMs = 0;
    qint64 trailingSilenceStartMs = 0;
    qint64 fadeOutStartMs = -1;
    double integratedLufs = -70.0;
    QList<double> shortTermLufs;
    bool beatsAttempted = false;
    bool beatsAvailable = false;
    QList<BeatRegion> beatRegions;
    QList<Structure::Result> structures;
    QList<KeyEstimator::Result> keys;

    bool valid() const;
    int headRegion() const;
    int tailRegion() const;
};

}
