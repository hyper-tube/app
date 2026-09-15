#include "BeatNetwork.h"

#include "Activation.h"
#include "Spectrogram.h"
#include "Tensor.h"
#include "core/Logging.h"

#include <algorithm>

namespace {

constexpr char kModelPath[] = ":/models/beat-this-small0.bin";
constexpr int kStemChannels = 32;
constexpr int kStemFrequencyTaps = 4;
constexpr int kStemTimeTaps = 3;
constexpr int kStemFrequencyStride = 4;
constexpr int kFrontendBlocks = 3;
constexpr int kHeadDimension = 32;
constexpr int kTransformerDimension = 128;
constexpr int kTransformerLayers = 6;
constexpr int kProjectionInputs = 1024;
constexpr int kOutputs = 2;
constexpr int kChunkFrames = 1500;
constexpr int kBorderFrames = 6;
constexpr float kMissingLogit = -1000.0f;

}

namespace analysis {

BeatNetwork::BeatNetwork()
    : m_weights(QString::fromLatin1(kModelPath))
    , m_inputNorm(m_weights, QStringLiteral("stem.input"), Spectrogram::kBands)
    , m_stem(m_weights, QStringLiteral("stem.conv"), 1, kStemChannels, kStemFrequencyTaps,
             kStemTimeTaps, kStemFrequencyStride)
    , m_stemNorm(m_weights, QStringLiteral("stem.norm"), kStemChannels)
    , m_projection(m_weights, QStringLiteral("frontend.linear"), kProjectionInputs,
                   kTransformerDimension, Linear::Bias::Present)
    , m_norm(m_weights, QStringLiteral("transformer.norm"), kTransformerDimension)
    , m_head(m_weights, QStringLiteral("head"), kTransformerDimension, kOutputs,
             Linear::Bias::Present)
{
    if (m_weights.valid()) {
        int channels = kStemChannels;
        for (int index = 0; index < kFrontendBlocks; ++index) {
            m_blocks.append(FrontendBlock(m_weights,
                                          QStringLiteral("frontend.") + QString::number(index),
                                          channels, kHeadDimension));
            channels *= 2;
        }
        for (int index = 0; index < kTransformerLayers; ++index) {
            m_layers.append(
                TransformerBlock(m_weights, QStringLiteral("transformer.") + QString::number(index),
                                 kTransformerDimension, kTransformerDimension / kHeadDimension));
        }
    }
    qCInfo(logTransition) << "beat model" << (valid() ? "ready" : "unavailable");
}

const BeatNetwork *BeatNetwork::shared()
{
    static const BeatNetwork network;
    static const bool available = network.valid();
    if (!available)
        return nullptr;
    return &network;
}

bool BeatNetwork::valid() const
{
    if (!m_weights.valid() || !m_inputNorm.valid() || !m_stem.valid() || !m_stemNorm.valid()
        || !m_projection.valid() || !m_norm.valid() || !m_head.valid())
        return false;
    if (m_blocks.size() != kFrontendBlocks || m_layers.size() != kTransformerLayers)
        return false;
    for (const FrontendBlock &block : m_blocks) {
        if (!block.valid())
            return false;
    }
    for (const TransformerBlock &layer : m_layers) {
        if (!layer.valid())
            return false;
    }
    return true;
}

BeatNetwork::Logits BeatNetwork::predict(const QList<float> &spectrogram, int frames) const
{
    Tensor activations(Spectrogram::kBands, 1, frames);
    for (int band = 0; band < Spectrogram::kBands; ++band) {
        float *values = activations.plane(band);
        for (int frame = 0; frame < frames; ++frame)
            values[frame] = spectrogram.at(qsizetype(frame) * Spectrogram::kBands + band);
    }
    m_inputNorm.apply(activations);

    activations.reshape(1, Spectrogram::kBands, frames);
    Tensor state = m_stem.apply(activations);
    m_stemNorm.apply(state);
    Activation::gelu(state.data(), state.size());
    for (const FrontendBlock &block : m_blocks)
        state = block.apply(state);

    const int channels = state.planes();
    const int bands = state.rows();
    if (channels * bands != kProjectionInputs)
        return {};

    Tensor sequence(1, frames, channels * bands);
    for (int channel = 0; channel < channels; ++channel) {
        for (int band = 0; band < bands; ++band) {
            const float *values = state.plane(channel) + qsizetype(band) * frames;
            float *target = sequence.data() + qsizetype(channel) * bands + band;
            for (int frame = 0; frame < frames; ++frame)
                target[qsizetype(frame) * channels * bands] = values[frame];
        }
    }

    Tensor projected(1, frames, kTransformerDimension);
    m_projection.apply(sequence.data(), projected.data(), frames);
    for (const TransformerBlock &layer : m_layers)
        layer.apply(projected);

    Tensor normalized(1, frames, kTransformerDimension);
    m_norm.apply(projected.data(), normalized.data(), frames);
    QList<float> outputs(qsizetype(frames) * kOutputs);
    m_head.apply(normalized.data(), outputs.data(), frames);

    Logits logits;
    logits.beat.resize(frames);
    logits.downbeat.resize(frames);
    for (int frame = 0; frame < frames; ++frame) {
        const float beat = outputs.at(qsizetype(frame) * kOutputs);
        const float downbeat = outputs.at(qsizetype(frame) * kOutputs + 1);
        logits.beat[frame] = beat + downbeat;
        logits.downbeat[frame] = downbeat;
    }
    return logits;
}

BeatNetwork::Logits BeatNetwork::run(const QList<float> &spectrogram, int frames,
                                     const std::atomic_bool &cancelled) const
{
    Logits logits;
    if (!valid() || frames < 1 || spectrogram.size() < qsizetype(frames) * Spectrogram::kBands)
        return logits;

    logits.beat.assign(frames, kMissingLogit);
    logits.downbeat.assign(frames, kMissingLogit);

    const int step = kChunkFrames - 2 * kBorderFrames;
    QList<int> starts;
    for (int start = -kBorderFrames; start < frames - kBorderFrames; start += step)
        starts.append(start);
    if (starts.isEmpty())
        starts.append(-kBorderFrames);
    if (frames > step)
        starts.last() = frames - (kChunkFrames - kBorderFrames);

    for (int index = starts.size() - 1; index >= 0; --index) {
        if (cancelled.load())
            return {};

        const int start = starts.at(index);
        const int begin = qMax(start, 0);
        const int end = qMin(start + kChunkFrames, frames);
        const int leading = qMax(0, -start);
        const int trailing = qMax(0, qMin(kBorderFrames, start + kChunkFrames - frames));
        const int length = leading + (end - begin) + trailing;
        if (length <= 2 * kBorderFrames)
            continue;

        QList<float> chunk(qsizetype(length) * Spectrogram::kBands, 0.0f);
        std::copy_n(spectrogram.constData() + qsizetype(begin) * Spectrogram::kBands,
                    qsizetype(end - begin) * Spectrogram::kBands,
                    chunk.data() + qsizetype(leading) * Spectrogram::kBands);

        const Logits predicted = predict(chunk, length);
        if (predicted.beat.size() != length || predicted.downbeat.size() != length)
            return {};

        for (int offset = kBorderFrames; offset < length - kBorderFrames; ++offset) {
            const int frame = start + offset;
            if (frame < 0 || frame >= frames)
                continue;
            logits.beat[frame] = predicted.beat.at(offset);
            logits.downbeat[frame] = predicted.downbeat.at(offset);
        }
    }
    return logits;
}

}
