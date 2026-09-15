#pragma once

#include <QString>

namespace analysis {

class ModelWeights;

class Linear
{
public:
    enum class Bias {
        Absent,
        Present,
    };

    Linear(const ModelWeights &weights, const QString &prefix, int inputs, int outputs, Bias bias);

    bool valid() const { return m_weight != nullptr && (m_bias != nullptr || !m_biased); }
    int inputs() const { return m_inputs; }
    int outputs() const { return m_outputs; }

    void apply(const float *input, float *output, int rows) const;

private:
    const float *m_weight = nullptr;
    const float *m_bias = nullptr;
    int m_inputs = 0;
    int m_outputs = 0;
    bool m_biased = false;
};

}
