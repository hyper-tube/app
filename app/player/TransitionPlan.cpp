#include "TransitionPlan.h"

namespace {

constexpr double kSlowestSpeed = 0.5;
constexpr double kFastestSpeed = 2.0;

QString fadeText(const player::Fade &fade)
{
    if (!fade.active())
        return QStringLiteral("none");
    return QStringLiteral("%1@%2+%3")
        .arg(fade.direction == player::Fade::In ? QStringLiteral("in") : QStringLiteral("out"))
        .arg(fade.startMs)
        .arg(fade.lengthMs);
}

}

namespace player {

bool TransitionPlan::valid() const
{
    return outgoingStartMs >= 0 && outgoingStopMs >= outgoingStartMs && handoffMs >= 0
        && lengthMs >= 0 && incomingStartMs >= 0 && incomingSpeed >= kSlowestSpeed
        && incomingSpeed <= kFastestSpeed && speedReturnMs >= 0 && speedReturnLengthMs >= 0
        && bars >= 0;
}

QString TransitionPlan::kindName() const
{
    switch (kind) {
    case Kind::Gapless: return QStringLiteral("gapless");
    case Kind::BeatMix: return QStringLiteral("beatmix");
    case Kind::Cut: return QStringLiteral("cut");
    case Kind::Fade: return QStringLiteral("fade");
    case Kind::Fixed: return QStringLiteral("fixed");
    }
    return QStringLiteral("fixed");
}

QString TransitionPlan::describe() const
{
    QStringList parts;
    parts.append(QStringLiteral("a_start_ms %1").arg(outgoingStartMs));
    parts.append(QStringLiteral("a_stop_ms %1").arg(outgoingStopMs));
    parts.append(QStringLiteral("handoff_ms %1").arg(handoffMs));
    parts.append(QStringLiteral("length_ms %1").arg(lengthMs));
    parts.append(QStringLiteral("bars %1").arg(bars));
    parts.append(QStringLiteral("a_fade %1").arg(fadeText(outgoingFade)));
    parts.append(QStringLiteral("a_bass %1").arg(fadeText(outgoingBassCut)));
    parts.append(QStringLiteral("b_start_ms %1").arg(incomingStartMs));
    parts.append(QStringLiteral("b_fade %1").arg(fadeText(incomingFade)));
    parts.append(QStringLiteral("b_bass %1").arg(fadeText(incomingBassEntry)));
    parts.append(QStringLiteral("b_speed %1").arg(incomingSpeed, 0, 'f', 4));
    parts.append(QStringLiteral("b_speed_return_ms %1").arg(speedReturnMs));
    parts.append(QStringLiteral("b_speed_return_length_ms %1").arg(speedReturnLengthMs));
    parts.append(QStringLiteral("locked %1").arg(phaseLocked ? 1 : 0));
    return parts.join(QLatin1Char(' '));
}

}
