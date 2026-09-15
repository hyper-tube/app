#pragma once

#include <QList>
#include <QString>

#include <array>

namespace analysis {

class KeyEstimator
{
public:
    struct Result
    {
        int tonic = -1;
        bool minor = false;
        double confidence = 0.0;

        bool valid() const;
        bool confident() const;
        int wheel() const;
        bool compatibleWith(const Result &other) const;
        QString name() const;
    };

    KeyEstimator();

    void add(const float *magnitudes);

    Result estimate() const;

private:
    QList<int> m_binIndex;
    QList<int> m_binClass;
    QList<float> m_binWeight;
    std::array<double, 12> m_chroma {};
};

}
