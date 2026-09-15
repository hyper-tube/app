#pragma once

#include <QList>

namespace analysis {

class Loudness
{
public:
    struct Result
    {
        qint64 leadingSilenceEndMs = 0;
        qint64 trailingSilenceStartMs = 0;
        qint64 fadeOutStartMs = -1;
        double integratedLufs = -70.0;
        QList<double> shortTermLufs;
    };

    static Result measure(const QList<float> &samples, int sampleRate);
};

}
