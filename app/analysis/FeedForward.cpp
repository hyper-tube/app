#include "FeedForward.h"

#include "Activation.h"
#include "ModelWeights.h"
#include "Tensor.h"

namespace {

constexpr int kInnerMultiplier = 4;
constexpr int kBlockRows = 4096;

}

namespace analysis {

FeedForward::FeedForward(const ModelWeights &weights, const QString &prefix, int dimension)
    : m_norm(weights, prefix + QStringLiteral(".norm"), dimension)
    , m_in(weights, prefix + QStringLiteral(".in"), dimension, dimension * kInnerMultiplier,
           Linear::Bias::Present)
    , m_out(weights, prefix + QStringLiteral(".out"), dimension * kInnerMultiplier, dimension,
            Linear::Bias::Present)
    , m_dimension(dimension)
    , m_inner(dimension * kInnerMultiplier)
{
}

bool FeedForward::valid() const
{
    return m_norm.valid() && m_in.valid() && m_out.valid();
}

void FeedForward::addTo(Tensor &activations) const
{
    if (!valid() || activations.columns() != m_dimension)
        return;

    const int rows = activations.planes() * activations.rows();
    QList<float> normalized(qsizetype(kBlockRows) * m_dimension);
    QList<float> hidden(qsizetype(kBlockRows) * m_inner);
    for (int block = 0; block < rows; block += kBlockRows) {
        const int height = qMin(kBlockRows, rows - block);
        float *values = activations.data() + qsizetype(block) * m_dimension;
        m_norm.apply(values, normalized.data(), height);
        m_in.apply(normalized.data(), hidden.data(), height);
        Activation::gelu(hidden.data(), qsizetype(height) * m_inner);
        m_out.apply(hidden.constData(), normalized.data(), height);
        const qsizetype total = qsizetype(height) * m_dimension;
        for (qsizetype index = 0; index < total; ++index)
            values[index] += normalized.at(index);
    }
}

}
