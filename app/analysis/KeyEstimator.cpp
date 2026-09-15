#include "KeyEstimator.h"

#include "Spectrogram.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace {

constexpr double kLowestHertz = 200.0;
constexpr double kHighestHertz = 5000.0;
constexpr double kSemitoneWidth = 1.0;
constexpr int kHarmonics = 4;
constexpr double kHarmonicDecay = 0.6;
constexpr double kReferenceHertz = 440.0;
constexpr double kReferenceMidi = 69.0;
constexpr int kClasses = 12;
constexpr int kFifth = 7;
constexpr int kRelativeMinorStep = 3;
constexpr double kMarginScale = 0.2;
constexpr double kConfidenceFloor = 0.5;

constexpr double kMajorProfile[kClasses] = {1.00, 0.00, 0.42, 0.00, 0.53, 0.37,
                                            0.00, 0.77, 0.00, 0.38, 0.21, 0.30};
constexpr double kMinorProfile[kClasses] = {1.00, 0.00, 0.36, 0.39, 0.00, 0.38,
                                            0.00, 0.74, 0.27, 0.00, 0.42, 0.23};

const char *const kNames[kClasses] = {"C",  "C#", "D",  "Eb", "E",  "F",
                                      "F#", "G",  "Ab", "A",  "Bb", "B"};

double circularDistance(double semitones)
{
    const double wrapped =
        std::fmod(std::fmod(semitones, double(kClasses)) + kClasses, double(kClasses));
    return std::min(wrapped, double(kClasses) - wrapped);
}

int wheelOf(int tonic, bool minor)
{
    const int major = minor ? (tonic + kRelativeMinorStep) % kClasses : tonic;
    return (major * kFifth) % kClasses;
}

double correlation(const std::array<double, kClasses> &chroma, const double *profile, int rotation)
{
    double chromaMean = 0.0;
    double profileMean = 0.0;
    for (int step = 0; step < kClasses; ++step) {
        chromaMean += chroma[step];
        profileMean += profile[step];
    }
    chromaMean /= kClasses;
    profileMean /= kClasses;

    double covariance = 0.0;
    double chromaEnergy = 0.0;
    double profileEnergy = 0.0;
    for (int step = 0; step < kClasses; ++step) {
        const double left = chroma[(step + rotation) % kClasses] - chromaMean;
        const double right = profile[step] - profileMean;
        covariance += left * right;
        chromaEnergy += left * left;
        profileEnergy += right * right;
    }
    if (chromaEnergy <= 0.0 || profileEnergy <= 0.0)
        return 0.0;
    return covariance / std::sqrt(chromaEnergy * profileEnergy);
}

}

namespace analysis {

bool KeyEstimator::Result::valid() const
{
    return tonic >= -1 && tonic < kClasses && confidence >= 0.0 && confidence <= 1.0;
}

bool KeyEstimator::Result::confident() const
{
    return tonic >= 0 && confidence >= kConfidenceFloor;
}

int KeyEstimator::Result::wheel() const
{
    return tonic < 0 ? -1 : wheelOf(tonic, minor);
}

bool KeyEstimator::Result::compatibleWith(const Result &other) const
{
    if (tonic < 0 || other.tonic < 0)
        return true;
    const int here = wheel();
    const int there = other.wheel();
    if (minor != other.minor)
        return here == there;
    const int forward = (here - there + kClasses) % kClasses;
    return std::min(forward, kClasses - forward) <= 1;
}

QString KeyEstimator::Result::name() const
{
    if (tonic < 0)
        return QStringLiteral("unknown");
    return QLatin1String(kNames[tonic])
        + (minor ? QStringLiteral(" minor") : QStringLiteral(" major"));
}

KeyEstimator::KeyEstimator()
{
    for (int bin = 1; bin < Spectrogram::kBins - 1; ++bin) {
        const double hertz = double(bin) * Spectrogram::kSampleRate / Spectrogram::kFftSize;
        if (hertz < kLowestHertz || hertz > kHighestHertz)
            continue;

        for (int pitchClass = 0; pitchClass < kClasses; ++pitchClass) {
            double weight = 0.0;
            for (int harmonic = 1; harmonic <= kHarmonics; ++harmonic) {
                const double midi = kReferenceMidi + kClasses * std::log2(hertz / kReferenceHertz)
                    - kClasses * std::log2(harmonic);
                const double distance = circularDistance(midi - pitchClass);
                if (distance >= kSemitoneWidth)
                    continue;
                const double window = std::cos(std::numbers::pi / 2.0 * distance / kSemitoneWidth);
                weight += std::pow(kHarmonicDecay, harmonic - 1) * window * window;
            }
            if (weight <= 0.0)
                continue;
            m_binIndex.append(bin);
            m_binClass.append(pitchClass);
            m_binWeight.append(float(weight));
        }
    }
}

void KeyEstimator::add(const float *magnitudes)
{
    std::array<double, kClasses> frame {};
    int currentBin = -1;
    bool peak = false;
    for (int entry = 0; entry < m_binIndex.size(); ++entry) {
        const int bin = m_binIndex.at(entry);
        if (bin != currentBin) {
            currentBin = bin;
            peak = magnitudes[bin] >= magnitudes[bin - 1] && magnitudes[bin] > magnitudes[bin + 1];
        }
        if (!peak)
            continue;
        frame[m_binClass.at(entry)] += double(m_binWeight.at(entry)) * double(magnitudes[bin]);
    }

    double energy = 0.0;
    for (const double value : frame)
        energy += value * value;
    if (energy <= 0.0)
        return;
    const double scale = std::sqrt(energy);
    for (int pitchClass = 0; pitchClass < kClasses; ++pitchClass)
        m_chroma[pitchClass] += frame[pitchClass] / scale;
}

KeyEstimator::Result KeyEstimator::estimate() const
{
    Result result;
    double total = 0.0;
    for (const double value : m_chroma)
        total += value;
    if (total <= 0.0)
        return result;

    double best = -2.0;
    for (int rotation = 0; rotation < kClasses; ++rotation) {
        const double major = correlation(m_chroma, kMajorProfile, rotation);
        if (major > best) {
            best = major;
            result.tonic = rotation;
            result.minor = false;
        }
        const double minor = correlation(m_chroma, kMinorProfile, rotation);
        if (minor > best) {
            best = minor;
            result.tonic = rotation;
            result.minor = true;
        }
    }
    if (result.tonic < 0)
        return result;

    double rival = -2.0;
    for (int rotation = 0; rotation < kClasses; ++rotation) {
        for (const bool minor : {false, true}) {
            if (wheelOf(rotation, minor) == result.wheel())
                continue;
            rival = std::max(
                rival, correlation(m_chroma, minor ? kMinorProfile : kMajorProfile, rotation));
        }
    }
    result.confidence = qBound(0.0, (best - rival) / kMarginScale, 1.0);
    return result;
}

}
