#pragma once

#include <QList>
#include <QString>

namespace analysis {

class ModelWeights;

class Rotary
{
public:
    Rotary(const ModelWeights &weights, const QString &name, int dimension);

    bool valid() const { return m_frequencies != nullptr; }

    void apply(float *values, int rows) const;

private:
    const float *m_frequencies = nullptr;
    int m_dimension = 0;
};

}
