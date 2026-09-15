#pragma once

#include "Fade.h"

#include <QString>
#include <QStringList>

namespace player {

class TransitionPlan
{
public:
    enum class Kind {
        Gapless,
        BeatMix,
        Cut,
        Fade,
        Fixed,
    };

    Kind kind = Kind::Fixed;
    qint64 outgoingStartMs = 0;
    qint64 outgoingStopMs = 0;
    qint64 handoffMs = 0;
    qint64 lengthMs = 0;
    player::Fade outgoingFade;
    player::Fade outgoingBassCut;
    qint64 incomingStartMs = 0;
    player::Fade incomingFade;
    player::Fade incomingBassEntry;
    double incomingSpeed = 1.0;
    qint64 speedReturnMs = 0;
    qint64 speedReturnLengthMs = 0;
    bool phaseLocked = false;
    int bars = 0;
    QStringList trace;

    bool valid() const;
    QString kindName() const;
    QString describe() const;
};

}
