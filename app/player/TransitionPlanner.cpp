#include "TransitionPlanner.h"

#include <array>
#include <cmath>
#include <numbers>

namespace {

using analysis::BeatRegion;
using analysis::KeyEstimator;
using analysis::Structure;
using analysis::TrackAnalysis;

constexpr qint64 kShortestOverlapMs = 1000;
constexpr qint64 kShortestCutFadeMs = 15;
constexpr qint64 kLongestCutFadeMs = 300;
constexpr qint64 kLoudEdgeOverlapMs = 4000;
constexpr qint64 kLoudEdgeWindowMs = 4000;
constexpr qint64 kQuietIntroFloorMs = 2000;
constexpr qint64 kCurveStepMs = 100;
constexpr double kDownbeatFloor = 0.5;
constexpr double kLoudEdgeLu = 3.0;
constexpr double kQuietIntroLu = 6.0;
constexpr double kMatchedTolerance = 0.04;
constexpr double kUnmatchedTolerance = 0.003;
constexpr int kSpeedReturnBars = 8;
constexpr int kShortestBars = 2;
constexpr qint64 kBarSlackDivisor = 50;
constexpr std::array<int, 3> kBarChoices = {16, 8, 4};

struct Side
{
    const BeatRegion *region = nullptr;
    const Structure::Result *structure = nullptr;
    const KeyEstimator::Result *key = nullptr;

    bool rhythmic() const
    {
        return region && structure && region->confident()
            && region->downbeatConfidence >= kDownbeatFloor && structure->available();
    }
};

Side sideAt(const TrackAnalysis &analysis, int index)
{
    Side side;
    if (index < 0 || index >= analysis.beatRegions.size())
        return side;
    side.region = &analysis.beatRegions.at(index);
    side.structure = &analysis.structures.at(index);
    side.key = &analysis.keys.at(index);
    return side;
}

double foldRatio(double ratio)
{
    if (!(ratio > 0.0))
        return 0.0;
    while (ratio > std::numbers::sqrt2)
        ratio /= 2.0;
    while (ratio < 1.0 / std::numbers::sqrt2)
        ratio *= 2.0;
    return ratio;
}

double curveLevel(const TrackAnalysis &analysis, qint64 startMs, qint64 endMs)
{
    const qsizetype first = qMax(qint64(0), startMs / kCurveStepMs);
    const qsizetype last = qMin(qsizetype(endMs / kCurveStepMs), analysis.shortTermLufs.size());
    if (first >= last)
        return -70.0;
    double total = 0.0;
    for (qsizetype index = first; index < last; ++index)
        total += analysis.shortTermLufs.at(index);
    return total / double(last - first);
}

qint64 quietIntroLength(const TrackAnalysis &analysis, qint64 startMs)
{
    const double floor = analysis.integratedLufs - kQuietIntroLu;
    const qsizetype first = qMax(qint64(0), startMs / kCurveStepMs);
    for (qsizetype index = first; index < analysis.shortTermLufs.size(); ++index) {
        if (analysis.shortTermLufs.at(index) >= floor)
            return qint64(index - first) * kCurveStepMs;
    }
    return 0;
}

qint64 firstDownbeatAfter(const Side &side, qint64 startMs)
{
    for (const qint64 downbeat : side.region->downbeatsMs) {
        if (downbeat >= startMs)
            return downbeat;
    }
    return -1;
}

bool nearPhrase(const Structure::Result &structure, qint64 time)
{
    for (const qint64 phrase : structure.phrasesMs) {
        if (std::abs(phrase - time) <= structure.barMs)
            return true;
    }
    return false;
}

QString keyText(const Side &side)
{
    if (!side.key || !side.key->valid() || side.key->tonic < 0)
        return QStringLiteral("unknown");
    return side.key->name().replace(QLatin1Char(' '), QLatin1Char('-'))
        + QStringLiteral("@%1").arg(side.key->confidence, 0, 'f', 2);
}

void traceCommon(player::TransitionPlan &plan, const player::TransitionPlanner::Context &context)
{
    plan.trace.append(QStringLiteral("mode %1").arg(int(context.mode)));
    plan.trace.append(QStringLiteral("cap_ms %1").arg(context.lengthCapMs));
    plan.trace.append(QStringLiteral("match_tempo %1").arg(context.matchTempo ? 1 : 0));
    plan.trace.append(QStringLiteral("same_album %1").arg(context.sameAlbum ? 1 : 0));
}

player::TransitionPlan fixedPlan(const std::optional<TrackAnalysis> &outgoing,
                                 const std::optional<TrackAnalysis> &incoming,
                                 const player::TransitionPlanner::Context &context, qint64 capMs,
                                 const QString &reason)
{
    player::TransitionPlan plan;
    plan.kind = player::TransitionPlan::Kind::Fixed;
    const qint64 duration = context.outgoingDurationMs > 0 ? context.outgoingDurationMs
                                                           : (outgoing ? outgoing->durationMs : 0);
    const qint64 stop = outgoing && outgoing->trailingSilenceStartMs > 0
        ? qBound(qint64(0), outgoing->trailingSilenceStartMs, duration)
        : duration;
    const qint64 entry =
        incoming ? qBound(qint64(0), incoming->leadingSilenceEndMs, incoming->durationMs) : 0;
    const qint64 start = qMax(qint64(0), stop - capMs);
    const qint64 length = stop - start;

    plan.outgoingStartMs = start;
    plan.outgoingStopMs = stop;
    plan.incomingStartMs = entry;
    plan.handoffMs = start;
    if (length >= kShortestOverlapMs) {
        plan.lengthMs = length;
        plan.outgoingFade = {player::Fade::Out, start, length};
        plan.incomingFade = {player::Fade::In, entry, length};
    } else {
        plan.handoffMs = stop;
        plan.outgoingStartMs = stop;
    }
    traceCommon(plan, context);
    plan.trace.append(QStringLiteral("duration_ms %1").arg(duration));
    plan.trace.append(QStringLiteral("reason %1").arg(reason));
    return plan;
}

player::TransitionPlan gaplessPlan(qint64 stopMs, qint64 incomingStartMs, const QString &reason,
                                   const player::TransitionPlanner::Context &context)
{
    player::TransitionPlan plan;
    plan.kind = player::TransitionPlan::Kind::Gapless;
    plan.outgoingStartMs = stopMs;
    plan.outgoingStopMs = stopMs;
    plan.handoffMs = stopMs;
    plan.incomingStartMs = incomingStartMs;
    traceCommon(plan, context);
    plan.trace.append(QStringLiteral("reason %1").arg(reason));
    return plan;
}

}

namespace player {

TransitionPlan TransitionPlanner::plan(const std::optional<TrackAnalysis> &outgoing,
                                       const std::optional<TrackAnalysis> &incoming,
                                       const Context &context)
{
    if (context.mode == PlaybackSettings::TransitionsOff)
        return fixedPlan(outgoing, incoming, context, 0, QStringLiteral("transitions_off"));
    if (context.mode == PlaybackSettings::Crossfade)
        return fixedPlan(outgoing, incoming, context, context.lengthCapMs,
                         QStringLiteral("crossfade_mode"));
    if (!outgoing || !incoming) {
        return fixedPlan(outgoing, incoming, context, context.lengthCapMs,
                         QStringLiteral("analysis_unavailable"));
    }

    const TrackAnalysis &a = *outgoing;
    const TrackAnalysis &b = *incoming;
    const qint64 stopMs = qBound(qint64(0), a.trailingSilenceStartMs, a.durationMs);
    const qint64 entryMs = qBound(qint64(0), b.leadingSilenceEndMs, b.durationMs);

    if (context.sameAlbum)
        return gaplessPlan(stopMs, entryMs, QStringLiteral("same_album"), context);
    if (context.lengthCapMs < kShortestOverlapMs)
        return gaplessPlan(stopMs, entryMs, QStringLiteral("cap_below_minimum"), context);

    const Side aSide = sideAt(a, a.tailRegion());
    const Side bSide = sideAt(b, b.headRegion());

    TransitionPlan plan;
    traceCommon(plan, context);
    plan.trace.append(QStringLiteral("a_stop_source trailing_silence"));
    plan.trace.append(QStringLiteral("beats_a %1").arg(aSide.rhythmic() ? 1 : 0));
    plan.trace.append(QStringLiteral("beats_b %1").arg(bSide.rhythmic() ? 1 : 0));
    if (aSide.region) {
        plan.trace.append(QStringLiteral("tempo_a %1").arg(aSide.region->tempo, 0, 'f', 2));
        plan.trace.append(
            QStringLiteral("beat_conf_a %1").arg(aSide.region->beatConfidence, 0, 'f', 2));
        plan.trace.append(
            QStringLiteral("downbeat_conf_a %1").arg(aSide.region->downbeatConfidence, 0, 'f', 2));
    }
    if (bSide.region) {
        plan.trace.append(QStringLiteral("tempo_b %1").arg(bSide.region->tempo, 0, 'f', 2));
        plan.trace.append(
            QStringLiteral("beat_conf_b %1").arg(bSide.region->beatConfidence, 0, 'f', 2));
        plan.trace.append(
            QStringLiteral("downbeat_conf_b %1").arg(bSide.region->downbeatConfidence, 0, 'f', 2));
    }
    plan.trace.append(QStringLiteral("key_a %1").arg(keyText(aSide)));
    plan.trace.append(QStringLiteral("key_b %1").arg(keyText(bSide)));

    const bool clash = aSide.key && bSide.key && aSide.key->confident() && bSide.key->confident()
        && !aSide.key->compatibleWith(*bSide.key);
    plan.trace.append(QStringLiteral("key_clash %1").arg(clash ? 1 : 0));

    if (aSide.rhythmic() && bSide.rhythmic()) {
        const double ratio = foldRatio(aSide.region->tempo / bSide.region->tempo);
        const double tolerance = context.matchTempo ? kMatchedTolerance : kUnmatchedTolerance;
        const qint64 barA = aSide.structure->barMs;
        const qint64 musicalEndA = aSide.structure->musicalEndMs;
        qint64 outroStartA =
            aSide.structure->outroStartMs >= 0 ? aSide.structure->outroStartMs : musicalEndA;
        if (a.fadeOutStartMs >= 0)
            outroStartA = qMin(outroStartA, a.fadeOutStartMs);
        const qint64 outroMsA = qMax(qint64(0), musicalEndA - outroStartA);
        const qint64 downbeatB = firstDownbeatAfter(bSide, entryMs);
        const qint64 introMsB =
            downbeatB >= 0 ? qMax(qint64(0), bSide.structure->introEndMs - downbeatB) : 0;

        plan.trace.append(QStringLiteral("ratio %1").arg(ratio, 0, 'f', 4));
        plan.trace.append(QStringLiteral("bar_a_ms %1").arg(barA));
        plan.trace.append(QStringLiteral("bar_b_ms %1").arg(bSide.structure->barMs));
        plan.trace.append(QStringLiteral("a_musical_end_ms %1").arg(musicalEndA));
        plan.trace.append(QStringLiteral("a_outro_ms %1").arg(outroMsA));
        plan.trace.append(QStringLiteral("b_intro_ms %1").arg(introMsB));
        plan.trace.append(QStringLiteral("bass_a %1").arg(aSide.structure->bassShare, 0, 'f', 2));
        plan.trace.append(QStringLiteral("bass_b %1").arg(bSide.structure->bassShare, 0, 'f', 2));

        if (std::abs(ratio - 1.0) <= tolerance && barA > 0 && downbeatB >= 0) {
            const qint64 barSlack = qMax(qint64(1), barA / kBarSlackDivisor);
            int bars = 0;
            for (const int candidate : kBarChoices) {
                const qint64 lengthA = qint64(candidate) * barA;
                const qint64 lengthB = qint64(std::llround(double(lengthA) * ratio));
                if (lengthA > context.lengthCapMs || lengthA > outroMsA + barSlack
                    || lengthB > introMsB + barSlack)
                    continue;
                bars = candidate;
                break;
            }
            if (bars > 0 && clash)
                bars = qMax(kShortestBars, bars / 2);
            const qint64 lengthA = qint64(bars) * barA;
            const qint64 startA = musicalEndA - lengthA;
            if (bars > 0 && startA >= 0) {
                const qint64 lengthB = qint64(std::llround(double(lengthA) * ratio));
                const qint64 beatA = barA / aSide.structure->beatsPerBar;
                const qint64 middleA = startA + qint64(bars / 2) * barA;
                const qint64 middleB =
                    downbeatB + qint64(std::llround(double(middleA - startA) * ratio));

                plan.kind = TransitionPlan::Kind::BeatMix;
                plan.bars = bars;
                plan.lengthMs = lengthA;
                plan.outgoingStartMs = startA;
                plan.outgoingStopMs = qMax(stopMs, musicalEndA);
                plan.outgoingFade = {Fade::Out, startA, lengthA};
                plan.outgoingBassCut = {Fade::Out, middleA, beatA};
                plan.incomingStartMs = downbeatB;
                plan.incomingFade = {Fade::In, downbeatB, lengthB};
                plan.incomingBassEntry = {Fade::In, middleB,
                                          qint64(std::llround(double(beatA) * ratio))};
                plan.incomingSpeed = ratio;
                plan.speedReturnMs = downbeatB + lengthB;
                plan.speedReturnLengthMs = qint64(kSpeedReturnBars) * bSide.structure->barMs;
                plan.phaseLocked = true;
                plan.handoffMs = middleA;
                plan.trace.append(QStringLiteral("a_phrase_aligned %1")
                                      .arg(nearPhrase(*aSide.structure, startA) ? 1 : 0));
                plan.trace.append(QStringLiteral("reason beatmix"));
                return plan;
            }
            plan.trace.append(QStringLiteral("beatmix_rejected no_length_fits"));
        } else {
            plan.trace.append(QStringLiteral("beatmix_rejected tempo_ratio"));
        }

        const qint64 lastDownbeatA = musicalEndA;
        const qint64 beatA = barA / aSide.structure->beatsPerBar;
        const double loudA = curveLevel(a, lastDownbeatA - barA, lastDownbeatA);
        const double loudB = curveLevel(b, downbeatB, downbeatB + bSide.structure->barMs);
        const bool edgesLoud = downbeatB >= 0 && loudA >= a.integratedLufs - kLoudEdgeLu
            && loudB >= b.integratedLufs - kLoudEdgeLu;
        plan.trace.append(QStringLiteral("last_bar_lufs_a %1").arg(loudA, 0, 'f', 1));
        plan.trace.append(QStringLiteral("first_bar_lufs_b %1").arg(loudB, 0, 'f', 1));

        const qint64 cutRoom = qMin(beatA / 2, downbeatB);
        const qint64 cutFadeMs =
            qBound(kShortestCutFadeMs, cutRoom, qMin(kLongestCutFadeMs, context.lengthCapMs));
        const bool cutFits = lastDownbeatA + beatA <= stopMs && downbeatB >= cutFadeMs;
        if (!edgesLoud)
            plan.trace.append(QStringLiteral("cut_rejected edges_quiet"));
        else if (!cutFits)
            plan.trace.append(QStringLiteral("cut_rejected no_room"));
        if (edgesLoud && cutFits) {
            const qint64 cutAt = lastDownbeatA + beatA;
            plan.kind = TransitionPlan::Kind::Cut;
            plan.lengthMs = cutFadeMs;
            plan.outgoingStartMs = cutAt - cutFadeMs;
            plan.outgoingStopMs = cutAt;
            plan.outgoingFade = {Fade::Out, cutAt - cutFadeMs, cutFadeMs};
            plan.incomingStartMs = downbeatB - cutFadeMs;
            plan.incomingFade = {Fade::In, downbeatB - cutFadeMs, cutFadeMs};
            plan.handoffMs = cutAt;
            plan.trace.append(QStringLiteral("beat_gap_ms %1").arg(beatA));
            plan.trace.append(QStringLiteral("cut_fade_ms %1").arg(cutFadeMs));
            plan.trace.append(QStringLiteral("reason cut"));
            return plan;
        }
    }

    const bool naturalFade =
        a.fadeOutStartMs >= 0 && a.fadeOutStartMs <= stopMs - kShortestOverlapMs;
    const double loudEndA = curveLevel(a, stopMs - kLoudEdgeWindowMs, stopMs);
    const double loudStartB = curveLevel(b, entryMs, entryMs + kLoudEdgeWindowMs);
    const bool loudEdges =
        loudEndA >= a.integratedLufs - kLoudEdgeLu && loudStartB >= b.integratedLufs - kLoudEdgeLu;
    const qint64 quietIntroMs = quietIntroLength(b, entryMs);

    qint64 length = context.lengthCapMs;
    QString reason = QStringLiteral("fade_default");
    if (naturalFade) {
        length = qMin(context.lengthCapMs, stopMs - a.fadeOutStartMs);
        reason = QStringLiteral("fade_natural");
    } else if (loudEdges) {
        length = qMin(context.lengthCapMs, kLoudEdgeOverlapMs);
        reason = QStringLiteral("fade_loud_edges");
    }
    length = qMin(length, stopMs);

    qint64 entry = length;
    if (quietIntroMs >= kQuietIntroFloorMs) {
        entry = qBound(length, qMax(length, quietIntroMs), qMin(context.lengthCapMs, stopMs));
        reason += QStringLiteral("+quiet_intro");
    }

    plan.kind = TransitionPlan::Kind::Fade;
    plan.lengthMs = entry;
    plan.outgoingStartMs = stopMs - entry;
    plan.outgoingStopMs = stopMs;
    plan.incomingStartMs = entryMs;
    plan.incomingFade = {Fade::In, entryMs, entry};
    if (!naturalFade)
        plan.outgoingFade = {Fade::Out, stopMs - length, length};
    if (loudEdges && !naturalFade) {
        const qint64 middle = stopMs - length / 2;
        const qint64 step = aSide.rhythmic() ? aSide.structure->barMs / aSide.structure->beatsPerBar
                                             : kShortestOverlapMs / 4;
        plan.outgoingBassCut = {Fade::Out, middle, step};
        plan.incomingBassEntry = {Fade::In, entryMs + (middle - plan.outgoingStartMs), step};
    }
    plan.handoffMs = plan.outgoingFade.active() ? plan.outgoingFade.startMs : plan.outgoingStartMs;
    plan.trace.append(QStringLiteral("natural_fade %1").arg(naturalFade ? 1 : 0));
    plan.trace.append(QStringLiteral("a_end_lufs %1").arg(loudEndA, 0, 'f', 1));
    plan.trace.append(QStringLiteral("a_integrated_lufs %1").arg(a.integratedLufs, 0, 'f', 1));
    plan.trace.append(QStringLiteral("b_start_lufs %1").arg(loudStartB, 0, 'f', 1));
    plan.trace.append(QStringLiteral("b_integrated_lufs %1").arg(b.integratedLufs, 0, 'f', 1));
    plan.trace.append(QStringLiteral("loud_edges %1").arg(loudEdges ? 1 : 0));
    plan.trace.append(QStringLiteral("quiet_intro_ms %1").arg(quietIntroMs));
    plan.trace.append(QStringLiteral("reason %1").arg(reason));
    return plan;
}

}
