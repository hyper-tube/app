#pragma once

#include "Linear.h"
#include "RmsNorm.h"

#include <QString>

namespace analysis {

class ModelWeights;
class Tensor;

class FeedForward
{
public:
    FeedForward(const ModelWeights &weights, const QString &prefix, int dimension);

    bool valid() const;

    void addTo(Tensor &activations) const;

private:
    RmsNorm m_norm;
    Linear m_in;
    Linear m_out;
    int m_dimension = 0;
    int m_inner = 0;
};

}
