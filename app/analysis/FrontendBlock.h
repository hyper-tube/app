#pragma once

#include "Attention.h"
#include "BatchNorm.h"
#include "Convolution.h"
#include "FeedForward.h"
#include "Tensor.h"

#include <QString>

namespace analysis {

class ModelWeights;

class FrontendBlock
{
public:
    FrontendBlock(const ModelWeights &weights, const QString &prefix, int channels,
                  int headDimension);

    bool valid() const;

    Tensor apply(const Tensor &activations) const;

private:
    Attention m_attentionFrequency;
    FeedForward m_forwardFrequency;
    Attention m_attentionTime;
    FeedForward m_forwardTime;
    Convolution m_convolution;
    BatchNorm m_norm;
    int m_channels = 0;
};

}
