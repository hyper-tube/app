#include "RmsNorm.h"

#include "ModelWeights.h"

#include <cmath>

namespace {

constexpr float kNormalizeFloor = 1e-12f;

}

namespace analysis {

RmsNorm::RmsNorm(const ModelWeights &weights, const QString &name, int size)
    : m_size(size)
{
    m_gamma = weights.values(name + QStringLiteral(".gamma"), {size});
}

void RmsNorm::apply(const float *input, float *output, int rows) const
{
    if (!valid())
        return;
    const float scale = std::sqrt(float(m_size));
    for (int row = 0; row < rows; ++row) {
        const float *source = input + qsizetype(row) * m_size;
        float *target = output + qsizetype(row) * m_size;
        float energy = 0.0f;
        for (int index = 0; index < m_size; ++index)
            energy += source[index] * source[index];
        const float inverse = scale / std::max(std::sqrt(energy), kNormalizeFloor);
        for (int index = 0; index < m_size; ++index)
            target[index] = source[index] * inverse * m_gamma[index];
    }
}

}
