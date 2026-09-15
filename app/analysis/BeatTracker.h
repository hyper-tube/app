#pragma once

#include "BeatRegion.h"
#include "KeyEstimator.h"
#include "Structure.h"

#include <QList>

#include <atomic>

namespace analysis {

class BeatTracker
{
public:
    struct Result
    {
        QList<BeatRegion> regions;
        QList<Structure::Result> structures;
        QList<KeyEstimator::Result> keys;
        bool modelAvailable = false;
        qint64 cpuMs = 0;
        qint64 frontendCpuMs = 0;
    };

    static Result track(const QList<float> &samples, int sampleRate,
                        const std::atomic_bool &cancelled);
};

}
