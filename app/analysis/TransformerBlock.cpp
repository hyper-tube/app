#include "TransformerBlock.h"

#include "ModelWeights.h"
#include "Tensor.h"

namespace analysis {

TransformerBlock::TransformerBlock(const ModelWeights &weights, const QString &prefix,
                                   int dimension, int heads)
    : m_attention(weights, prefix + QStringLiteral(".attention"), dimension, heads)
    , m_forward(weights, prefix + QStringLiteral(".forward"), dimension)
{
}

bool TransformerBlock::valid() const
{
    return m_attention.valid() && m_forward.valid();
}

void TransformerBlock::apply(Tensor &activations) const
{
    m_attention.addTo(activations);
    m_forward.addTo(activations);
}

}
