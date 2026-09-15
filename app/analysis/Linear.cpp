#include "Linear.h"

#include "Gemm.h"
#include "ModelWeights.h"

namespace analysis {

Linear::Linear(const ModelWeights &weights, const QString &prefix, int inputs, int outputs,
               Bias bias)
    : m_inputs(inputs)
    , m_outputs(outputs)
    , m_biased(bias == Bias::Present)
{
    m_weight = weights.values(prefix + QStringLiteral(".weight"), {inputs, outputs});
    if (m_biased)
        m_bias = weights.values(prefix + QStringLiteral(".bias"), {outputs});
}

void Linear::apply(const float *input, float *output, int rows) const
{
    if (!valid())
        return;
    if (m_biased)
        Gemm::multiplyBiased(input, m_weight, m_bias, output, rows, m_inputs, m_outputs);
    else
        Gemm::multiply(input, m_weight, output, rows, m_inputs, m_outputs);
}

}
