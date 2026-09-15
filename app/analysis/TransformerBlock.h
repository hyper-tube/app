#pragma once

#include "Attention.h"
#include "FeedForward.h"

#include <QString>

namespace analysis {

class ModelWeights;
class Tensor;

class TransformerBlock
{
public:
    TransformerBlock(const ModelWeights &weights, const QString &prefix, int dimension, int heads);

    bool valid() const;

    void apply(Tensor &activations) const;

private:
    Attention m_attention;
    FeedForward m_forward;
};

}
