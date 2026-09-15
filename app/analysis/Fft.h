#pragma once

#include <QList>

namespace analysis {

class Fft
{
public:
    explicit Fft(int size);

    int size() const { return m_size; }

    void transform(float *real, float *imaginary) const;

private:
    int m_size = 0;
    QList<int> m_reversed;
    QList<float> m_cosine;
    QList<float> m_sine;
};

}
