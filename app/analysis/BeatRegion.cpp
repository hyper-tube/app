#include "BeatRegion.h"

namespace {

constexpr double kConfidenceFloor = 0.5;
constexpr double kSlowestTempo = 40.0;
constexpr double kFastestTempo = 240.0;

}

namespace analysis {

bool BeatRegion::valid() const
{
    return endMs > startMs && tempo >= 0.0 && phaseMs >= 0.0 && beatConfidence >= 0.0
        && beatConfidence <= 1.0 && downbeatConfidence >= 0.0 && downbeatConfidence <= 1.0
        && downbeatsMs.size() <= beatsMs.size();
}

bool BeatRegion::confident() const
{
    return beatConfidence >= kConfidenceFloor && tempo >= kSlowestTempo && tempo <= kFastestTempo;
}

}
