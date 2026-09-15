#include "Spectrogram.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace {

constexpr double kLowestFrequency = 30.0;
constexpr double kHighestFrequency = 11000.0;
constexpr double kLogMultiplier = 1000.0;
constexpr double kLinearMelStep = 200.0 / 3.0;
constexpr double kLogMelFrequency = 1000.0;
constexpr double kLogMelDecades = 6.4;
constexpr double kLogMelBands = 27.0;
constexpr int kNyquistHertz = analysis::Spectrogram::kSampleRate / 2;

double hertzToMel(double hertz)
{
    const double linear = hertz / kLinearMelStep;
    if (hertz < kLogMelFrequency)
        return linear;
    const double step = std::log(kLogMelDecades) / kLogMelBands;
    return kLogMelFrequency / kLinearMelStep + std::log(hertz / kLogMelFrequency) / step;
}

double melToHertz(double mel)
{
    const double boundary = kLogMelFrequency / kLinearMelStep;
    if (mel < boundary)
        return mel * kLinearMelStep;
    const double step = std::log(kLogMelDecades) / kLogMelBands;
    return kLogMelFrequency * std::exp(step * (mel - boundary));
}

float sampleAt(const QList<float> &samples, qsizetype index)
{
    const qsizetype last = samples.size() - 1;
    if (last < 0)
        return 0.0f;
    if (last == 0)
        return samples.at(0);
    const qsizetype period = 2 * last;
    const qsizetype folded = ((index % period) + period) % period;
    return samples.at(folded <= last ? folded : period - folded);
}

}

namespace analysis {

Spectrogram::Spectrogram()
    : m_fft(kFftSize)
{
    m_window.resize(kFftSize);
    for (int index = 0; index < kFftSize; ++index)
        m_window[index] = float(0.5 - 0.5 * std::cos(2.0 * std::numbers::pi * index / kFftSize));

    const int bins = kFftSize / 2 + 1;
    QList<double> frequencies(bins);
    for (int index = 0; index < bins; ++index)
        frequencies[index] = double(kNyquistHertz) * index / (bins - 1);

    QList<double> points(kBands + 2);
    const double lowest = hertzToMel(kLowestFrequency);
    const double highest = hertzToMel(kHighestFrequency);
    for (int index = 0; index < points.size(); ++index)
        points[index] = melToHertz(lowest + (highest - lowest) * index / (points.size() - 1));

    m_bandStart.resize(kBands);
    m_bandLength.resize(kBands);
    m_bandOffset.resize(kBands);
    for (int band = 0; band < kBands; ++band) {
        const double lower = points.at(band);
        const double center = points.at(band + 1);
        const double upper = points.at(band + 2);
        int start = -1;
        int length = 0;
        m_bandOffset[band] = int(m_bandWeights.size());
        for (int index = 0; index < bins; ++index) {
            const double rising = (frequencies.at(index) - lower) / (center - lower);
            const double falling = (upper - frequencies.at(index)) / (upper - center);
            const double weight = std::max(0.0, std::min(rising, falling));
            if (weight <= 0.0) {
                if (start >= 0)
                    break;
                continue;
            }
            if (start < 0)
                start = index;
            ++length;
            m_bandWeights.append(float(weight));
        }
        m_bandStart[band] = qMax(0, start);
        m_bandLength[band] = length;
    }
}

int Spectrogram::frameCount(qsizetype samples)
{
    if (samples <= 0)
        return 0;
    return int(samples / kHopSize) + 1;
}

QList<float> Spectrogram::frames(const QList<float> &samples, int first, int count,
                                 const FrameObserver &observe) const
{
    QList<float> result;
    if (count <= 0 || first < 0 || m_fft.size() != kFftSize)
        return result;

    result.resize(qsizetype(count) * kBands);
    QList<float> real(kFftSize);
    QList<float> imaginary(kFftSize);
    QList<float> magnitudes(kBins);
    const float normalize = 1.0f / std::sqrt(float(kFftSize));

    for (int frame = 0; frame < count; ++frame) {
        const qsizetype center = qsizetype(first + frame) * kHopSize - kFftSize / 2;
        for (int index = 0; index < kFftSize; ++index)
            real[index] = m_window.at(index) * sampleAt(samples, center + index);
        imaginary.fill(0.0f);
        m_fft.transform(real.data(), imaginary.data());
        for (int bin = 0; bin < magnitudes.size(); ++bin)
            magnitudes[bin] = std::hypot(real.at(bin), imaginary.at(bin)) * normalize;
        if (observe)
            observe(magnitudes.constData());

        float *target = result.data() + qsizetype(frame) * kBands;
        for (int band = 0; band < kBands; ++band) {
            const float *weights = m_bandWeights.constData() + m_bandOffset.at(band);
            const int start = m_bandStart.at(band);
            double total = 0.0;
            for (int index = 0; index < m_bandLength.at(band); ++index)
                total += double(weights[index]) * magnitudes.at(start + index);
            target[band] = float(std::log1p(kLogMultiplier * total));
        }
    }
    return result;
}

}
