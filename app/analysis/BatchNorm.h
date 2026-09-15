#pragma once

#include <QString>

namespace analysis {

class ModelWeights;
class Tensor;

class BatchNorm
{
public:
    BatchNorm(const ModelWeights &weights, const QString &prefix, int channels);

    bool valid() const;

    void apply(Tensor &activations) const;

private:
    const float *m_weight = nullptr;
    const float *m_bias = nullptr;
    const float *m_mean = nullptr;
    const float *m_variance = nullptr;
    int m_channels = 0;
};

}
