#include "TrackAnalysis.h"

namespace analysis {

bool TrackAnalysis::valid() const
{
    if (version != kVersion || videoId.isEmpty() || durationMs <= 0 || shortTermLufs.isEmpty())
        return false;
    if (structures.size() != beatRegions.size() || keys.size() != beatRegions.size())
        return false;
    for (const BeatRegion &region : beatRegions) {
        if (!region.valid())
            return false;
    }
    for (const Structure::Result &structure : structures) {
        if (!structure.valid())
            return false;
    }
    for (const KeyEstimator::Result &key : keys) {
        if (!key.valid())
            return false;
    }
    return beatsAvailable || beatRegions.isEmpty();
}

int TrackAnalysis::headRegion() const
{
    return beatRegions.isEmpty() ? -1 : 0;
}

int TrackAnalysis::tailRegion() const
{
    return beatRegions.isEmpty() ? -1 : int(beatRegions.size()) - 1;
}

}
