#pragma once

#include "player/StreamResolver.h"

#include <QList>

#include <atomic>
#include <optional>

namespace analysis {

class PcmReader
{
public:
    struct Result
    {
        QList<float> samples;
        qint64 durationMs = 0;
        qint64 elapsedMs = 0;
    };

    static constexpr int kSampleRate = 22050;

    static std::optional<Result> read(const player::Stream &stream,
                                      const std::atomic_bool &cancelled);
};

}
