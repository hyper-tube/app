#pragma once

#include "TrackAnalysis.h"

#include <optional>

namespace analysis {

class AnalysisStore
{
public:
    std::optional<TrackAnalysis> load(const QString &videoId, int itag) const;
    bool save(const TrackAnalysis &analysis) const;
};

}
