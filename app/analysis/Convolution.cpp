#include "Convolution.h"

#include "ModelWeights.h"

namespace {

constexpr int kTimePadding = 1;

}

namespace analysis {

Convolution::Convolution(const ModelWeights &weights, const QString &name, int inputChannels,
                         int outputChannels, int frequencyTaps, int timeTaps, int frequencyStride)
    : m_inputChannels(inputChannels)
    , m_outputChannels(outputChannels)
    , m_frequencyTaps(frequencyTaps)
    , m_timeTaps(timeTaps)
    , m_frequencyStride(frequencyStride)
{
    m_weight = weights.values(name + QStringLiteral(".weight"),
                              {outputChannels, inputChannels * frequencyTaps * timeTaps});
}

Tensor Convolution::apply(const Tensor &activations) const
{
    const int time = activations.columns();
    const int frequencies = (activations.rows() - m_frequencyTaps) / m_frequencyStride + 1;
    if (!valid() || activations.planes() != m_inputChannels || frequencies < 1)
        return {};

    Tensor result(m_outputChannels, frequencies, time);
    for (int output = 0; output < m_outputChannels; ++output) {
        for (int frequency = 0; frequency < frequencies; ++frequency) {
            float *target = result.plane(output) + qsizetype(frequency) * time;
            for (int channel = 0; channel < m_inputChannels; ++channel) {
                const float *kernel = m_weight
                    + ((qsizetype(output) * m_inputChannels + channel) * m_frequencyTaps)
                        * m_timeTaps;
                for (int tap = 0; tap < m_frequencyTaps; ++tap) {
                    const float *source = activations.plane(channel)
                        + qsizetype(frequency * m_frequencyStride + tap) * time;
                    for (int step = 0; step < m_timeTaps; ++step) {
                        const float scale = kernel[qsizetype(tap) * m_timeTaps + step];
                        const int shift = step - kTimePadding;
                        const int begin = qMax(0, -shift);
                        const int end = qMin(time, time - shift);
                        for (int index = begin; index < end; ++index)
                            target[index] += scale * source[index + shift];
                    }
                }
            }
        }
    }
    return result;
}

}
