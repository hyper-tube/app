#pragma once

#include <QString>

namespace analysis {

class ModelWeights;

class RmsNorm
{
public:
    RmsNorm(const ModelWeights &weights, const QString &name, int size);

    bool valid() const { return m_gamma != nullptr; }

    void apply(const float *input, float *output, int rows) const;

private:
    const float *m_gamma = nullptr;
    int m_size = 0;
};

}
