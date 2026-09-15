#include "FrontendBlock.h"

#include "Activation.h"
#include "ModelWeights.h"

namespace {

constexpr int kFrequencyTaps = 2;
constexpr int kTimeTaps = 3;
constexpr int kFrequencyStride = 2;

analysis::Tensor toTimeMajor(const analysis::Tensor &source)
{
    const int channels = source.planes();
    const int frequencies = source.rows();
    const int time = source.columns();
    analysis::Tensor result(time, frequencies, channels);
    for (int channel = 0; channel < channels; ++channel) {
        for (int frequency = 0; frequency < frequencies; ++frequency) {
            const float *values = source.plane(channel) + qsizetype(frequency) * time;
            for (int step = 0; step < time; ++step)
                result.plane(step)[qsizetype(frequency) * channels + channel] = values[step];
        }
    }
    return result;
}

analysis::Tensor toFrequencyMajor(const analysis::Tensor &source)
{
    const int time = source.planes();
    const int frequencies = source.rows();
    const int channels = source.columns();
    analysis::Tensor result(frequencies, time, channels);
    for (int step = 0; step < time; ++step) {
        for (int frequency = 0; frequency < frequencies; ++frequency) {
            const float *values = source.plane(step) + qsizetype(frequency) * channels;
            std::copy_n(values, channels, result.plane(frequency) + qsizetype(step) * channels);
        }
    }
    return result;
}

analysis::Tensor toChannelMajor(const analysis::Tensor &source)
{
    const int frequencies = source.planes();
    const int time = source.rows();
    const int channels = source.columns();
    analysis::Tensor result(channels, frequencies, time);
    for (int frequency = 0; frequency < frequencies; ++frequency) {
        for (int step = 0; step < time; ++step) {
            const float *values = source.plane(frequency) + qsizetype(step) * channels;
            for (int channel = 0; channel < channels; ++channel)
                result.plane(channel)[qsizetype(frequency) * time + step] = values[channel];
        }
    }
    return result;
}

}

namespace analysis {

FrontendBlock::FrontendBlock(const ModelWeights &weights, const QString &prefix, int channels,
                             int headDimension)
    : m_attentionFrequency(weights, prefix + QStringLiteral(".attentionF"), channels,
                           channels / headDimension)
    , m_forwardFrequency(weights, prefix + QStringLiteral(".forwardF"), channels)
    , m_attentionTime(weights, prefix + QStringLiteral(".attentionT"), channels,
                      channels / headDimension)
    , m_forwardTime(weights, prefix + QStringLiteral(".forwardT"), channels)
    , m_convolution(weights, prefix + QStringLiteral(".conv"), channels, channels * 2,
                    kFrequencyTaps, kTimeTaps, kFrequencyStride)
    , m_norm(weights, prefix + QStringLiteral(".norm"), channels * 2)
    , m_channels(channels)
{
}

bool FrontendBlock::valid() const
{
    return m_attentionFrequency.valid() && m_forwardFrequency.valid() && m_attentionTime.valid()
        && m_forwardTime.valid() && m_convolution.valid() && m_norm.valid();
}

Tensor FrontendBlock::apply(const Tensor &activations) const
{
    if (!valid() || activations.planes() != m_channels)
        return {};

    Tensor overFrequency = toTimeMajor(activations);
    m_attentionFrequency.addTo(overFrequency);
    m_forwardFrequency.addTo(overFrequency);

    Tensor overTime = toFrequencyMajor(overFrequency);
    m_attentionTime.addTo(overTime);
    m_forwardTime.addTo(overTime);

    Tensor result = m_convolution.apply(toChannelMajor(overTime));
    m_norm.apply(result);
    Activation::gelu(result.data(), result.size());
    return result;
}

}
