#pragma once

#include "Fft.h"

#include <QList>

#include <functional>

namespace analysis {

class Spectrogram
{
public:
    using FrameObserver = std::function<void(const float *magnitudes)>;

    static constexpr int kSampleRate = 22050;
    static constexpr int kFftSize = 1024;
    static constexpr int kHopSize = 441;
    static constexpr int kBands = 128;
    static constexpr int kBins = kFftSize / 2 + 1;
    static constexpr int kFramesPerSecond = kSampleRate / kHopSize;

    Spectrogram();

    static int frameCount(qsizetype samples);

    QList<float> frames(const QList<float> &samples, int first, int count,
                        const FrameObserver &observe = {}) const;

private:
    Fft m_fft;
    QList<float> m_window;
    QList<int> m_bandStart;
    QList<int> m_bandLength;
    QList<int> m_bandOffset;
    QList<float> m_bandWeights;
};

}
