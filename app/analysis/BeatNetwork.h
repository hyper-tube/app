#pragma once

#include "BatchNorm.h"
#include "Convolution.h"
#include "FrontendBlock.h"
#include "Linear.h"
#include "ModelWeights.h"
#include "RmsNorm.h"
#include "TransformerBlock.h"

#include <QList>

#include <atomic>

namespace analysis {

class BeatNetwork
{
public:
    struct Logits
    {
        QList<float> beat;
        QList<float> downbeat;
    };

    static const BeatNetwork *shared();

    BeatNetwork(const BeatNetwork &) = delete;
    BeatNetwork &operator=(const BeatNetwork &) = delete;

    bool valid() const;

    Logits run(const QList<float> &spectrogram, int frames,
               const std::atomic_bool &cancelled) const;

private:
    BeatNetwork();

    Logits predict(const QList<float> &spectrogram, int frames) const;

    ModelWeights m_weights;
    BatchNorm m_inputNorm;
    Convolution m_stem;
    BatchNorm m_stemNorm;
    QList<FrontendBlock> m_blocks;
    Linear m_projection;
    QList<TransformerBlock> m_layers;
    RmsNorm m_norm;
    Linear m_head;
};

}
