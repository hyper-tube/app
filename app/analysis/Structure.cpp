#include "Structure.h"

#include "Spectrogram.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr double kLowEdgeHertz = 150.0;
constexpr double kHighEdgeHertz = 2000.0;
constexpr double kEnergyFloor = 1e-12;
constexpr int kPhraseBars = 4;
constexpr int kPhraseRadius = 2;
constexpr double kPhraseNoveltyDb = 3.0;
constexpr int kMinimumBars = 4;
constexpr int kDefaultBeatsPerBar = 4;
constexpr int kShortestBeatsPerBar = 2;
constexpr int kLongestBeatsPerBar = 8;
constexpr double kLoudQuantile = 0.8;
constexpr double kFullDb = 2.0;
constexpr double kEdgeDropDb = 3.0;
constexpr int kSnapBars = 2;

int binFor(double hertz)
{
    return int(
        std::lround(hertz * analysis::Spectrogram::kFftSize / analysis::Spectrogram::kSampleRate));
}

double decibels(double energy)
{
    return 10.0 * std::log10(energy + kEnergyFloor);
}

double mean(const QList<double> &values, int first, int last)
{
    double total = 0.0;
    int count = 0;
    for (int index = qMax(0, first); index <= qMin(int(values.size()) - 1, last); ++index) {
        total += values.at(index);
        ++count;
    }
    return count > 0 ? total / count : 0.0;
}

double quantile(QList<double> values, double share)
{
    if (values.isEmpty())
        return 0.0;
    std::ranges::sort(values);
    return values.at(qBound(0, int(share * (values.size() - 1)), int(values.size()) - 1));
}

double medianOf(QList<double> values)
{
    if (values.isEmpty())
        return 0.0;
    std::ranges::sort(values);
    const int middle = int(values.size()) / 2;
    if (values.size() % 2 == 1)
        return values.at(middle);
    return 0.5 * (values.at(middle - 1) + values.at(middle));
}

int beatsPerBarOf(const analysis::BeatRegion &region, double periodMs)
{
    if (region.downbeatsMs.size() < 3)
        return kDefaultBeatsPerBar;
    QList<double> intervals;
    for (int index = 1; index < region.downbeatsMs.size(); ++index)
        intervals.append(double(region.downbeatsMs.at(index) - region.downbeatsMs.at(index - 1)));
    const int beats = int(std::lround(medianOf(intervals) / periodMs));
    if (beats < kShortestBeatsPerBar || beats > kLongestBeatsPerBar)
        return kDefaultBeatsPerBar;
    return beats;
}

int anchorResidue(const analysis::BeatRegion &region, double periodMs, int beatsPerBar)
{
    QList<int> votes(beatsPerBar, 0);
    for (const qint64 downbeat : region.downbeatsMs) {
        const double index = std::round((double(downbeat) - region.phaseMs) / periodMs);
        const int residue = int(std::fmod(index, double(beatsPerBar)) + beatsPerBar) % beatsPerBar;
        ++votes[residue];
    }
    int best = 0;
    for (int residue = 1; residue < beatsPerBar; ++residue) {
        if (votes.at(residue) > votes.at(best))
            best = residue;
    }
    return best;
}

qint64 snapToPhrase(qint64 time, const QList<qint64> &phrases, qint64 barMs)
{
    for (const qint64 phrase : phrases) {
        if (std::abs(phrase - time) <= kSnapBars * barMs)
            return phrase;
    }
    return time;
}

}

namespace analysis {

bool Structure::Result::available() const
{
    return !barsMs.isEmpty() && barMs > 0 && beatsPerBar > 0;
}

bool Structure::Result::valid() const
{
    if (introEndMs < -1 || outroStartMs < -1 || musicalEndMs < -1 || bassShare < 0.0
        || bassShare > 1.0)
        return false;
    if (barsMs.isEmpty()) {
        return barMs == 0 && beatsPerBar == 0 && lowDb.isEmpty() && midDb.isEmpty()
            && highDb.isEmpty() && phrasesMs.isEmpty();
    }
    return barMs > 0 && beatsPerBar >= kShortestBeatsPerBar && beatsPerBar <= kLongestBeatsPerBar
        && lowDb.size() == barsMs.size() && midDb.size() == barsMs.size()
        && highDb.size() == barsMs.size() && musicalEndMs > barsMs.constFirst();
}

double Structure::Result::levelDb(int bar) const
{
    if (bar < 0 || bar >= barsMs.size())
        return decibels(0.0);
    return decibels(std::pow(10.0, lowDb.at(bar) / 10.0) + std::pow(10.0, midDb.at(bar) / 10.0)
                    + std::pow(10.0, highDb.at(bar) / 10.0));
}

double Structure::Result::loudDb() const
{
    QList<double> levels;
    levels.reserve(barsMs.size());
    for (int bar = 0; bar < barsMs.size(); ++bar)
        levels.append(levelDb(bar));
    return quantile(levels, kLoudQuantile);
}

Structure::Structure(qint64 startMs)
    : m_startMs(startMs)
{
}

void Structure::add(const float *magnitudes)
{
    static const int lowEnd = binFor(kLowEdgeHertz);
    static const int highStart = binFor(kHighEdgeHertz);

    double low = 0.0;
    double mid = 0.0;
    double high = 0.0;
    for (int bin = 1; bin < lowEnd; ++bin)
        low += double(magnitudes[bin]) * double(magnitudes[bin]);
    for (int bin = lowEnd; bin < highStart; ++bin)
        mid += double(magnitudes[bin]) * double(magnitudes[bin]);
    for (int bin = highStart; bin < Spectrogram::kBins; ++bin)
        high += double(magnitudes[bin]) * double(magnitudes[bin]);
    m_low.append(float(low));
    m_mid.append(float(mid));
    m_high.append(float(high));
}

Structure::Result Structure::measure(const BeatRegion &region) const
{
    Result result;
    if (region.tempo <= 0.0 || region.beatsMs.isEmpty() || m_low.isEmpty())
        return result;

    const double periodMs = 60000.0 / region.tempo;
    const int beatsPerBar = beatsPerBarOf(region, periodMs);
    const double barMs = periodMs * beatsPerBar;
    const int residue = anchorResidue(region, periodMs, beatsPerBar);
    const qint64 limit =
        qMin(region.endMs, region.beatsMs.constLast() + qint64(std::llround(periodMs)));

    const qint64 firstIndex =
        qint64(std::ceil((double(region.startMs) - region.phaseMs) / periodMs));
    const qint64 aligned =
        firstIndex + ((residue - firstIndex) % beatsPerBar + beatsPerBar) % beatsPerBar;

    QList<qint64> starts;
    for (qint64 index = aligned;
         region.phaseMs + double(index + beatsPerBar) * periodMs <= double(limit);
         index += beatsPerBar) {
        starts.append(qint64(std::llround(region.phaseMs + double(index) * periodMs)));
    }
    if (starts.size() < kMinimumBars)
        return result;

    const qint64 barLength = qint64(std::llround(barMs));
    double lowTotal = 0.0;
    double allTotal = 0.0;
    for (const qint64 start : starts) {
        const int first = int((start - m_startMs) * Spectrogram::kFramesPerSecond / 1000);
        const int last =
            int((start + barLength - m_startMs) * Spectrogram::kFramesPerSecond / 1000);
        if (first < 0 || last > m_low.size() || last <= first)
            return {};

        double low = 0.0;
        double mid = 0.0;
        double high = 0.0;
        for (int frame = first; frame < last; ++frame) {
            low += m_low.at(frame);
            mid += m_mid.at(frame);
            high += m_high.at(frame);
        }
        const double frames = last - first;
        result.barsMs.append(start);
        result.lowDb.append(decibels(low / frames));
        result.midDb.append(decibels(mid / frames));
        result.highDb.append(decibels(high / frames));
        lowTotal += low;
        allTotal += low + mid + high;
    }

    result.barMs = barLength;
    result.beatsPerBar = beatsPerBar;
    result.musicalEndMs = starts.constLast() + barLength;
    result.bassShare = allTotal > 0.0 ? qBound(0.0, lowTotal / allTotal, 1.0) : 0.0;

    const int bars = int(result.barsMs.size());
    QList<double> novelty(bars, 0.0);
    for (int bar = kPhraseBars; bar + kPhraseBars <= bars; ++bar) {
        const double low = mean(result.lowDb, bar, bar + kPhraseBars - 1)
            - mean(result.lowDb, bar - kPhraseBars, bar - 1);
        const double mid = mean(result.midDb, bar, bar + kPhraseBars - 1)
            - mean(result.midDb, bar - kPhraseBars, bar - 1);
        const double high = mean(result.highDb, bar, bar + kPhraseBars - 1)
            - mean(result.highDb, bar - kPhraseBars, bar - 1);
        novelty[bar] = std::sqrt(low * low + mid * mid + high * high);
    }
    for (int bar = kPhraseBars; bar + kPhraseBars <= bars; ++bar) {
        if (novelty.at(bar) < kPhraseNoveltyDb)
            continue;
        bool highest = true;
        for (int other = qMax(0, bar - kPhraseRadius);
             other <= qMin(bars - 1, bar + kPhraseRadius) && highest; ++other)
            highest = novelty.at(other) <= novelty.at(bar);
        if (highest)
            result.phrasesMs.append(result.barsMs.at(bar));
    }

    QList<double> levels;
    levels.reserve(bars);
    for (int bar = 0; bar < bars; ++bar)
        levels.append(result.levelDb(bar));
    const double loud = quantile(levels, kLoudQuantile);

    result.introEndMs = result.barsMs.constFirst();
    for (int bar = 1; bar < bars; ++bar) {
        const double after = mean(levels, bar, bar + kPhraseBars - 1);
        const double before = mean(levels, 0, bar - 1);
        if (after < loud - kFullDb || before > after - kEdgeDropDb)
            continue;
        result.introEndMs = snapToPhrase(result.barsMs.at(bar), result.phrasesMs, barLength);
        break;
    }

    result.outroStartMs = result.musicalEndMs;
    for (int bar = bars - 1; bar >= 1; --bar) {
        const double after = mean(levels, bar, bars - 1);
        const double before = mean(levels, bar - kPhraseBars, bar - 1);
        if (before < loud - kFullDb || after > before - kEdgeDropDb)
            continue;
        result.outroStartMs = snapToPhrase(result.barsMs.at(bar), result.phrasesMs, barLength);
        break;
    }
    return result;
}

}
