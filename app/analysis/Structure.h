#pragma once

#include "BeatRegion.h"

#include <QList>

namespace analysis {

class Structure
{
public:
    struct Result
    {
        qint64 barMs = 0;
        int beatsPerBar = 0;
        QList<qint64> barsMs;
        QList<double> lowDb;
        QList<double> midDb;
        QList<double> highDb;
        QList<qint64> phrasesMs;
        qint64 introEndMs = -1;
        qint64 outroStartMs = -1;
        qint64 musicalEndMs = -1;
        double bassShare = 0.0;

        bool available() const;
        bool valid() const;
        double levelDb(int bar) const;
        double loudDb() const;
    };

    explicit Structure(qint64 startMs);

    void add(const float *magnitudes);

    Result measure(const BeatRegion &region) const;

private:
    qint64 m_startMs = 0;
    QList<float> m_low;
    QList<float> m_mid;
    QList<float> m_high;
};

}
