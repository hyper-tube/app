#include "BeatTracker.h"

#include "BeatNetwork.h"
#include "KeyEstimator.h"
#include "Spectrogram.h"
#include "Structure.h"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <numbers>

namespace {

constexpr int kHeadSeconds = 60;
constexpr int kTailSeconds = 75;
constexpr int kPeakRadius = 3;
constexpr int kMinimumBeats = 8;
constexpr int kPeriodCandidates = 4000;
constexpr double kLowestRatio = 0.7;
constexpr double kHighestRatio = 1.4;
constexpr double kSlowestPeriod = 1.5;
constexpr double kFastestPeriod = 0.25;
constexpr double kInlierWindow = 0.25;
constexpr double kMatchWindow = 0.25;
constexpr int kShortestBar = 2;
constexpr int kLongestBar = 8;

struct Region
{
    int first = 0;
    int last = 0;
};

struct Grid
{
    double period = 0.0;
    double offset = 0.0;
    double confidence = 0.0;
};

QList<Region> regionsFor(int frames)
{
    const int head = qMin(frames, kHeadSeconds * analysis::Spectrogram::kFramesPerSecond);
    const int tail = qMax(0, frames - kTailSeconds * analysis::Spectrogram::kFramesPerSecond);
    if (tail >= head)
        return {{0, head}, {tail, frames}};
    return {{0, frames}};
}

QList<double> pickPeaks(const QList<float> &logits)
{
    QList<int> peaks;
    for (int index = 0; index < logits.size(); ++index) {
        const float value = logits.at(index);
        if (value <= 0.0f)
            continue;
        const int begin = qMax(0, index - kPeakRadius);
        const int end = qMin(int(logits.size()) - 1, index + kPeakRadius);
        bool highest = true;
        for (int other = begin; other <= end && highest; ++other)
            highest = logits.at(other) <= value;
        if (highest)
            peaks.append(index);
    }

    QList<double> merged;
    if (peaks.isEmpty())
        return merged;
    double current = peaks.first();
    int count = 1;
    for (int index = 1; index < peaks.size(); ++index) {
        const double next = peaks.at(index);
        if (next - current <= 1.0) {
            ++count;
            current += (next - current) / count;
        } else {
            merged.append(current);
            current = next;
            count = 1;
        }
    }
    merged.append(current);
    return merged;
}

QList<double> framesToSeconds(const QList<double> &frames)
{
    QList<double> times;
    times.reserve(frames.size());
    for (const double frame : frames)
        times.append(frame / analysis::Spectrogram::kFramesPerSecond);
    return times;
}

QList<double> snapToBeats(const QList<double> &downbeats, const QList<double> &beats)
{
    QList<double> snapped;
    if (beats.isEmpty())
        return snapped;
    for (const double time : downbeats) {
        double best = beats.first();
        for (const double beat : beats) {
            if (std::abs(beat - time) < std::abs(best - time))
                best = beat;
        }
        if (snapped.isEmpty() || snapped.constLast() != best)
            snapped.append(best);
    }
    return snapped;
}

double median(QList<double> values)
{
    if (values.isEmpty())
        return 0.0;
    std::ranges::sort(values);
    const int middle = int(values.size()) / 2;
    if (values.size() % 2 == 1)
        return values.at(middle);
    return 0.5 * (values.at(middle - 1) + values.at(middle));
}

QList<double> intervalsOf(const QList<double> &times)
{
    QList<double> intervals;
    for (int index = 1; index < times.size(); ++index)
        intervals.append(times.at(index) - times.at(index - 1));
    return intervals;
}

struct Coherence
{
    double period = 0.0;
    double offset = 0.0;
    double strength = 0.0;
};

Coherence strongestPeriod(const QList<double> &times, double typical)
{
    Coherence best;
    for (int step = 0; step < kPeriodCandidates; ++step) {
        const double ratio =
            kLowestRatio + (kHighestRatio - kLowestRatio) * step / double(kPeriodCandidates - 1);
        const double period = typical * ratio;
        double real = 0.0;
        double imaginary = 0.0;
        for (const double time : times) {
            const double angle = 2.0 * std::numbers::pi * time / period;
            real += std::cos(angle);
            imaginary += std::sin(angle);
        }
        real /= double(times.size());
        imaginary /= double(times.size());
        const double strength = std::hypot(real, imaginary);
        if (strength <= best.strength)
            continue;
        best.strength = strength;
        best.period = period;
        best.offset = std::atan2(imaginary, real) / (2.0 * std::numbers::pi) * period;
    }
    return best;
}

QList<double> gridIndices(const QList<double> &times, double period, double offset)
{
    QList<double> indices;
    indices.reserve(times.size());
    for (const double time : times)
        indices.append(std::round((time - offset) / period));
    return indices;
}

bool refit(const QList<double> &times, const QList<double> &indices, const QList<bool> &inliers,
           double &period, double &offset)
{
    double count = 0.0;
    double sumIndex = 0.0;
    double sumSquare = 0.0;
    double sumTime = 0.0;
    double sumProduct = 0.0;
    for (int position = 0; position < times.size(); ++position) {
        if (!inliers.at(position))
            continue;
        const double index = indices.at(position);
        count += 1.0;
        sumIndex += index;
        sumSquare += index * index;
        sumTime += times.at(position);
        sumProduct += index * times.at(position);
    }
    if (count < kMinimumBeats)
        return false;
    const double denominator = count * sumSquare - sumIndex * sumIndex;
    if (std::abs(denominator) < 1e-9)
        return false;
    const double fitted = (count * sumProduct - sumIndex * sumTime) / denominator;
    if (fitted < kFastestPeriod || fitted > kSlowestPeriod)
        return false;
    period = fitted;
    offset = (sumTime - period * sumIndex) / count;
    return true;
}

QList<bool> selectInliers(const QList<double> &times, const QList<double> &indices, double period,
                          double offset)
{
    QList<bool> inliers;
    inliers.reserve(times.size());
    for (int position = 0; position < times.size(); ++position) {
        const double error = times.at(position) - (offset + period * indices.at(position));
        inliers.append(std::abs(error) <= kInlierWindow * period);
    }
    return inliers;
}

Grid fitGrid(const QList<double> &times)
{
    Grid grid;
    if (times.size() < kMinimumBeats)
        return grid;

    const double typical = median(intervalsOf(times));
    if (typical < kFastestPeriod || typical > kSlowestPeriod)
        return grid;

    const Coherence coherence = strongestPeriod(times, typical);
    if (coherence.period < kFastestPeriod || coherence.period > kSlowestPeriod)
        return grid;

    double period = coherence.period;
    double offset = coherence.offset;
    QList<double> indices = gridIndices(times, period, offset);
    QList<bool> inliers = selectInliers(times, indices, period, offset);
    if (refit(times, indices, inliers, period, offset)) {
        indices = gridIndices(times, period, offset);
        inliers = selectInliers(times, indices, period, offset);
    }

    double lowest = 0.0;
    double highest = 0.0;
    double count = 0.0;
    for (int position = 0; position < times.size(); ++position) {
        if (!inliers.at(position))
            continue;
        const double index = indices.at(position);
        lowest = count > 0.0 ? qMin(lowest, index) : index;
        highest = count > 0.0 ? qMax(highest, index) : index;
        count += 1.0;
    }
    if (count < kMinimumBeats)
        return grid;

    const double coverage = qBound(0.0, count / (highest - lowest + 1.0), 1.0);
    grid.period = period;
    grid.offset = offset;
    grid.confidence = qBound(0.0, coherence.strength * coverage, 1.0);
    return grid;
}

double barConfidence(const QList<double> &downbeats, double period)
{
    if (downbeats.size() < 3 || period <= 0.0)
        return 0.0;
    const QList<double> intervals = intervalsOf(downbeats);
    const double typical = median(intervals);
    const double beats = std::round(typical / period);
    if (beats < kShortestBar || beats > kLongestBar)
        return 0.0;
    int agreeing = 0;
    for (const double interval : intervals) {
        if (std::abs(interval - typical) <= kMatchWindow * period)
            ++agreeing;
    }
    return double(agreeing) / double(intervals.size());
}

qint64 toMilliseconds(double seconds)
{
    return qint64(std::llround(seconds * 1000.0));
}

}

namespace analysis {

BeatTracker::Result BeatTracker::track(const QList<float> &samples, int sampleRate,
                                       const std::atomic_bool &cancelled)
{
    Result result;
    if (sampleRate != Spectrogram::kSampleRate || samples.isEmpty())
        return result;

    const std::clock_t started = std::clock();
    const BeatNetwork *network = BeatNetwork::shared();
    if (!network)
        return result;
    result.modelAvailable = true;

    static const Spectrogram spectrogram;
    const int frames = Spectrogram::frameCount(samples.size());
    for (const Region &region : regionsFor(frames)) {
        if (cancelled.load())
            return {};
        const int count = region.last - region.first;
        if (count < 1)
            continue;

        const double offset = double(region.first) / Spectrogram::kFramesPerSecond;
        Structure structure(toMilliseconds(offset));
        KeyEstimator key;

        const std::clock_t frontendStarted = std::clock();
        const QList<float> frames =
            spectrogram.frames(samples, region.first, count, [&structure, &key](const float *bins) {
            structure.add(bins);
            key.add(bins);
        });
        result.frontendCpuMs +=
            qint64(1000.0 * double(std::clock() - frontendStarted) / CLOCKS_PER_SEC);

        const BeatNetwork::Logits logits = network->run(frames, count, cancelled);
        if (logits.beat.isEmpty())
            return {};

        const QList<double> beats = framesToSeconds(pickPeaks(logits.beat));
        const QList<double> downbeats =
            snapToBeats(framesToSeconds(pickPeaks(logits.downbeat)), beats);

        BeatRegion tracked;
        tracked.startMs = toMilliseconds(offset);
        tracked.endMs = toMilliseconds(double(region.last) / Spectrogram::kFramesPerSecond);
        for (const double beat : beats)
            tracked.beatsMs.append(toMilliseconds(beat + offset));
        for (const double downbeat : downbeats)
            tracked.downbeatsMs.append(toMilliseconds(downbeat + offset));

        const Grid grid = fitGrid(beats);
        if (grid.period > 0.0) {
            tracked.tempo = 60.0 / grid.period;
            const double phase = std::fmod(grid.offset + offset, grid.period);
            tracked.phaseMs = 1000.0 * (phase < 0.0 ? phase + grid.period : phase);
            tracked.beatConfidence = grid.confidence;
            tracked.downbeatConfidence = grid.confidence * barConfidence(downbeats, grid.period);
        }
        result.regions.append(tracked);
        result.structures.append(structure.measure(tracked));
        result.keys.append(key.estimate());
    }

    result.cpuMs = qint64(1000.0 * double(std::clock() - started) / CLOCKS_PER_SEC);
    return result;
}

}
