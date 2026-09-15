#include "Loudness.h"

#include <QtGlobal>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace {

constexpr double kAbsoluteGateLufs = -70.0;
constexpr double kSilenceLufs = -50.0;
constexpr int kWindowMilliseconds = 400;
constexpr int kStepMilliseconds = 100;
constexpr int kFadeMinimumMilliseconds = 1000;
constexpr int kFadeDropWindowMilliseconds = 2000;
constexpr double kFadeDropLufs = 3.0;
constexpr double kFadeStartLufs = 0.5;

class Biquad
{
public:
    void setHighShelf(double sampleRate, double frequency, double gainDb, double q)
    {
        const double amplitude = std::pow(10.0, gainDb / 40.0);
        const double omega = 2.0 * std::numbers::pi * frequency / sampleRate;
        const double sine = std::sin(omega);
        const double cosine = std::cos(omega);
        const double alpha = sine / (2.0 * q);
        const double beta = 2.0 * std::sqrt(amplitude) * alpha;
        set(amplitude * ((amplitude + 1.0) + (amplitude - 1.0) * cosine + beta),
            -2.0 * amplitude * ((amplitude - 1.0) + (amplitude + 1.0) * cosine),
            amplitude * ((amplitude + 1.0) + (amplitude - 1.0) * cosine - beta),
            (amplitude + 1.0) - (amplitude - 1.0) * cosine + beta,
            2.0 * ((amplitude - 1.0) - (amplitude + 1.0) * cosine),
            (amplitude + 1.0) - (amplitude - 1.0) * cosine - beta);
    }

    void setHighPass(double sampleRate, double frequency, double q)
    {
        const double omega = 2.0 * std::numbers::pi * frequency / sampleRate;
        const double sine = std::sin(omega);
        const double cosine = std::cos(omega);
        const double alpha = sine / (2.0 * q);
        set((1.0 + cosine) / 2.0, -(1.0 + cosine), (1.0 + cosine) / 2.0, 1.0 + alpha, -2.0 * cosine,
            1.0 - alpha);
    }

    double apply(double sample)
    {
        const double output = m_b0 * sample + m_b1 * m_x1 + m_b2 * m_x2 - m_a1 * m_y1 - m_a2 * m_y2;
        m_x2 = m_x1;
        m_x1 = sample;
        m_y2 = m_y1;
        m_y1 = output;
        return output;
    }

private:
    void set(double b0, double b1, double b2, double a0, double a1, double a2)
    {
        m_b0 = b0 / a0;
        m_b1 = b1 / a0;
        m_b2 = b2 / a0;
        m_a1 = a1 / a0;
        m_a2 = a2 / a0;
    }

    double m_b0 = 1.0;
    double m_b1 = 0.0;
    double m_b2 = 0.0;
    double m_a1 = 0.0;
    double m_a2 = 0.0;
    double m_x1 = 0.0;
    double m_x2 = 0.0;
    double m_y1 = 0.0;
    double m_y2 = 0.0;
};

double lufsForEnergy(double energy)
{
    if (energy <= 0.0)
        return kAbsoluteGateLufs;
    return qMax(kAbsoluteGateLufs, -0.691 + 10.0 * std::log10(energy));
}

double integratedLufs(const QList<double> &energies)
{
    QList<double> gated;
    gated.reserve(energies.size());
    for (const double energy : energies) {
        if (lufsForEnergy(energy) > kAbsoluteGateLufs)
            gated.append(energy);
    }
    if (gated.isEmpty())
        return kAbsoluteGateLufs;

    const double preliminary =
        std::accumulate(gated.cbegin(), gated.cend(), 0.0) / double(gated.size());
    const double relativeGate = lufsForEnergy(preliminary) - 10.0;
    gated.removeIf([relativeGate](double energy) { return lufsForEnergy(energy) < relativeGate; });
    if (gated.isEmpty())
        return kAbsoluteGateLufs;
    return lufsForEnergy(std::accumulate(gated.cbegin(), gated.cend(), 0.0) / double(gated.size()));
}

qint64 leadingSilenceEnd(const QList<double> &curve)
{
    for (int index = 0; index < curve.size(); ++index) {
        if (curve.at(index) >= kSilenceLufs)
            return qint64(index) * kStepMilliseconds;
    }
    return qint64(curve.size()) * kStepMilliseconds;
}

qint64 trailingSilenceStart(const QList<double> &curve, qint64 durationMs)
{
    for (int index = curve.size() - 1; index >= 0; --index) {
        if (curve.at(index) >= kSilenceLufs)
            return qMin(durationMs, qint64(index + 1) * kStepMilliseconds);
    }
    return 0;
}

qint64 fadeOutStart(const QList<double> &curve, qint64 silenceStartMs)
{
    const int end = qMin(curve.size(), int(silenceStartMs / kStepMilliseconds));
    const int minimum = kFadeMinimumMilliseconds / kStepMilliseconds;
    if (end < minimum)
        return -1;

    double peak = kAbsoluteGateLufs;
    for (int index = 0; index < end; ++index)
        peak = qMax(peak, curve.at(index));
    if (peak < kSilenceLufs)
        return -1;

    const int dropWindow = kFadeDropWindowMilliseconds / kStepMilliseconds;
    for (int index = 0; index + minimum < end; ++index) {
        if (curve.at(index) > peak - kFadeStartLufs)
            continue;
        const int dropAt = qMin(end - 1, index + dropWindow);
        if (curve.at(dropAt) > curve.at(index) - kFadeDropLufs)
            continue;

        bool declining = true;
        for (int later = index + 1; later < end; ++later) {
            if (curve.at(later) > curve.at(later - 1) + 1.5) {
                declining = false;
                break;
            }
        }
        if (declining)
            return qint64(index) * kStepMilliseconds;
    }
    return -1;
}

}

namespace analysis {

Loudness::Result Loudness::measure(const QList<float> &samples, int sampleRate)
{
    Result result;
    if (samples.isEmpty() || sampleRate <= 0)
        return result;

    Biquad shelf;
    Biquad highPass;
    shelf.setHighShelf(sampleRate, 1681.974, 4.0, 0.707175);
    highPass.setHighPass(sampleRate, 38.135, 0.500327);

    const qsizetype windowSamples = qMax(1, sampleRate * kWindowMilliseconds / 1000);
    const qsizetype stepSamples = qMax(1, sampleRate * kStepMilliseconds / 1000);
    QList<double> energies;
    QList<double> silenceCurve;
    QList<double> window(windowSamples, 0.0);
    double windowTotal = 0.0;
    double blockTotal = 0.0;
    qsizetype blockLength = 0;
    for (qsizetype index = 0; index < samples.size(); ++index) {
        const double weighted = highPass.apply(shelf.apply(samples.at(index)));
        const double energy = weighted * weighted;
        const qsizetype slot = index % windowSamples;
        windowTotal += energy - window.at(slot);
        window[slot] = energy;
        blockTotal += energy;
        ++blockLength;

        const qsizetype covered = index + 1;
        if (covered >= windowSamples && (covered - windowSamples) % stepSamples == 0)
            energies.append(windowTotal / double(windowSamples));
        if (blockLength == stepSamples || covered == samples.size()) {
            silenceCurve.append(lufsForEnergy(blockTotal / double(blockLength)));
            blockTotal = 0.0;
            blockLength = 0;
        }
    }

    result.shortTermLufs.reserve(energies.size());
    for (const double energy : energies)
        result.shortTermLufs.append(lufsForEnergy(energy));
    const qint64 durationMs = qint64(samples.size()) * 1000 / sampleRate;
    result.integratedLufs = integratedLufs(energies);
    result.leadingSilenceEndMs = leadingSilenceEnd(silenceCurve);
    result.trailingSilenceStartMs = trailingSilenceStart(silenceCurve, durationMs);
    result.fadeOutStartMs = fadeOutStart(result.shortTermLufs, result.trailingSilenceStartMs);
    return result;
}

}
