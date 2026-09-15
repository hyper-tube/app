#pragma once

#include "Tensor.h"

#include <QString>

namespace analysis {

class ModelWeights;

class Convolution
{
public:
    Convolution(const ModelWeights &weights, const QString &name, int inputChannels,
                int outputChannels, int frequencyTaps, int timeTaps, int frequencyStride);

    bool valid() const { return m_weight != nullptr; }

    Tensor apply(const Tensor &activations) const;

private:
    const float *m_weight = nullptr;
    int m_inputChannels = 0;
    int m_outputChannels = 0;
    int m_frequencyTaps = 0;
    int m_timeTaps = 0;
    int m_frequencyStride = 0;
};

}
