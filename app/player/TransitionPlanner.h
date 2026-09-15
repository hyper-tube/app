#pragma once

#include "PlaybackSettings.h"
#include "TransitionPlan.h"
#include "analysis/TrackAnalysis.h"

#include <optional>

namespace player {

class TransitionPlanner
{
public:
    struct Context
    {
        PlaybackSettings::TransitionMode mode = PlaybackSettings::TransitionsOff;
        qint64 lengthCapMs = 0;
        qint64 outgoingDurationMs = 0;
        bool matchTempo = true;
        bool sameAlbum = false;
    };

    static TransitionPlan plan(const std::optional<analysis::TrackAnalysis> &outgoing,
                               const std::optional<analysis::TrackAnalysis> &incoming,
                               const Context &context);
};

}
