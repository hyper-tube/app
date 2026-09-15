#include "BatchNorm.h"

#include "ModelWeights.h"
#include "Tensor.h"

#include <cmath>

namespace {

constexpr float kEpsilon = 1e-5f;

}

namespace analysis {

BatchNorm::BatchNorm(const ModelWeights &weights, const QString &prefix, int channels)
    : m_channels(channels)
{
    m_weight = weights.values(prefix + QStringLiteral(".weight"), {channels});
    m_bias = weights.values(prefix + QStringLiteral(".bias"), {channels});
    m_mean = weights.values(prefix + QStringLiteral(".mean"), {channels});
    m_variance = weights.values(prefix + QStringLiteral(".variance"), {channels});
}

bool BatchNorm::valid() const
{
    return m_weight != nullptr && m_bias != nullptr && m_mean != nullptr && m_variance != nullptr;
}

void BatchNorm::apply(Tensor &activations) const
{
    if (!valid() || activations.planes() != m_channels)
        return;
    const int count = activations.planeSize();
    for (int channel = 0; channel < m_channels; ++channel) {
        const float scale = m_weight[channel] / std::sqrt(m_variance[channel] + kEpsilon);
        const float shift = m_bias[channel] - m_mean[channel] * scale;
        float *values = activations.plane(channel);
        for (int index = 0; index < count; ++index)
            values[index] = values[index] * scale + shift;
    }
}

}
