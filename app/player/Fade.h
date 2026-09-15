#pragma once

#include <QtGlobal>

namespace player {

struct Fade
{
    enum Direction {
        In,
        Out,
    };

    Direction direction = In;
    qint64 startMs = 0;
    qint64 lengthMs = 0;

    bool active() const { return lengthMs > 0; }
    bool operator==(const Fade &other) const = default;
};

}
