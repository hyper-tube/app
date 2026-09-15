#include "Rotary.h"

#include "ModelWeights.h"

#include <cmath>

namespace analysis {

Rotary::Rotary(const ModelWeights &weights, const QString &name, int dimension)
    : m_dimension(dimension)
{
    m_frequencies = weights.values(name + QStringLiteral(".frequencies"), {dimension / 2});
}

void Rotary::apply(float *values, int rows) const
{
    if (!valid())
        return;
    const int pairs = m_dimension / 2;
    for (int row = 0; row < rows; ++row) {
        float *target = values + qsizetype(row) * m_dimension;
        for (int pair = 0; pair < pairs; ++pair) {
            const float angle = float(row) * m_frequencies[pair];
            const float cosine = std::cos(angle);
            const float sine = std::sin(angle);
            float *rotated = target + qsizetype(pair) * 2;
            const float even = rotated[0];
            const float odd = rotated[1];
            rotated[0] = even * cosine - odd * sine;
            rotated[1] = odd * cosine + even * sine;
        }
    }
}

}
