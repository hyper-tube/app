#pragma once

#include "Linear.h"
#include "RmsNorm.h"
#include "Rotary.h"

#include <QString>

namespace analysis {

class ModelWeights;
class Tensor;

class Attention
{
public:
    Attention(const ModelWeights &weights, const QString &prefix, int dimension, int heads);

    bool valid() const;

    void addTo(Tensor &activations) const;

private:
    void addGroup(Tensor &activations, int first, int sequences) const;

    RmsNorm m_norm;
    Linear m_qkv;
    Linear m_gates;
    Linear m_out;
    Rotary m_rotary;
    int m_dimension = 0;
    int m_heads = 0;
    int m_headDimension = 0;
};

}
