#include "Attention.h"

#include "Gemm.h"
#include "ModelWeights.h"
#include "Tensor.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr int kQueryBlock = 96;
constexpr int kGroupRows = 4096;

void softmax(float *values, int rows, int columns)
{
    for (int row = 0; row < rows; ++row) {
        float *target = values + qsizetype(row) * columns;
        float largest = target[0];
        for (int index = 1; index < columns; ++index)
            largest = qMax(largest, target[index]);
        float total = 0.0f;
        for (int index = 0; index < columns; ++index) {
            const float value = std::exp(target[index] - largest);
            target[index] = value;
            total += value;
        }
        const float inverse = 1.0f / total;
        for (int index = 0; index < columns; ++index)
            target[index] *= inverse;
    }
}

float sigmoid(float value)
{
    return 1.0f / (1.0f + std::exp(-value));
}

}

namespace analysis {

Attention::Attention(const ModelWeights &weights, const QString &prefix, int dimension, int heads)
    : m_norm(weights, prefix + QStringLiteral(".norm"), dimension)
    , m_qkv(weights, prefix + QStringLiteral(".qkv"), dimension, dimension * 3,
            Linear::Bias::Absent)
    , m_gates(weights, prefix + QStringLiteral(".gates"), dimension, heads, Linear::Bias::Present)
    , m_out(weights, prefix + QStringLiteral(".out"), dimension, dimension, Linear::Bias::Absent)
    , m_rotary(weights, prefix + QStringLiteral(".rotary"), dimension / heads)
    , m_dimension(dimension)
    , m_heads(heads)
    , m_headDimension(dimension / heads)
{
}

bool Attention::valid() const
{
    return m_norm.valid() && m_qkv.valid() && m_gates.valid() && m_out.valid() && m_rotary.valid();
}

void Attention::addTo(Tensor &activations) const
{
    if (!valid() || activations.columns() != m_dimension || activations.rows() < 1)
        return;

    const int group = qMax(1, kGroupRows / activations.rows());
    for (int first = 0; first < activations.planes(); first += group)
        addGroup(activations, first, qMin(group, activations.planes() - first));
}

void Attention::addGroup(Tensor &activations, int first, int sequences) const
{
    const int length = activations.rows();
    const int rows = sequences * length;
    const int width = m_dimension * 3;
    const qsizetype span = qsizetype(length) * m_headDimension;

    QList<float> normalized(qsizetype(rows) * m_dimension);
    QList<float> mixed(qsizetype(rows) * width);
    QList<float> gates(qsizetype(rows) * m_heads);
    QList<float> context(qsizetype(rows) * m_dimension);
    float *values = activations.plane(first);
    m_norm.apply(values, normalized.data(), rows);
    m_qkv.apply(normalized.data(), mixed.data(), rows);
    m_gates.apply(normalized.data(), gates.data(), rows);

    const float scale = 1.0f / std::sqrt(float(m_headDimension));
    QList<float> query(span);
    QList<float> key(span);
    QList<float> value(span);
    QList<float> keyTransposed(span);
    QList<float> scores(qsizetype(kQueryBlock) * length);
    QList<float> attended(qsizetype(kQueryBlock) * m_headDimension);

    for (int sequence = 0; sequence < sequences; ++sequence) {
        const float *source = mixed.constData() + qsizetype(sequence) * length * width;
        const float *sequenceGates = gates.constData() + qsizetype(sequence) * length * m_heads;
        float *sequenceContext = context.data() + qsizetype(sequence) * length * m_dimension;
        for (int head = 0; head < m_heads; ++head) {
            const int offset = head * m_headDimension;
            for (int position = 0; position < length; ++position) {
                const float *row = source + qsizetype(position) * width + offset;
                const qsizetype target = qsizetype(position) * m_headDimension;
                std::copy_n(row, m_headDimension, query.data() + target);
                std::copy_n(row + m_dimension, m_headDimension, key.data() + target);
                std::copy_n(row + qsizetype(m_dimension) * 2, m_headDimension,
                            value.data() + target);
            }
            m_rotary.apply(query.data(), length);
            m_rotary.apply(key.data(), length);
            for (int position = 0; position < length; ++position) {
                for (int index = 0; index < m_headDimension; ++index) {
                    query[qsizetype(position) * m_headDimension + index] *= scale;
                    keyTransposed[qsizetype(index) * length + position] =
                        key.at(qsizetype(position) * m_headDimension + index);
                }
            }

            for (int block = 0; block < length; block += kQueryBlock) {
                const int height = qMin(kQueryBlock, length - block);
                Gemm::multiply(query.constData() + qsizetype(block) * m_headDimension,
                               keyTransposed.constData(), scores.data(), height, m_headDimension,
                               length);
                softmax(scores.data(), height, length);
                Gemm::multiply(scores.constData(), value.constData(), attended.data(), height,
                               length, m_headDimension);
                for (int row = 0; row < height; ++row) {
                    const int position = block + row;
                    const float gate = sigmoid(sequenceGates[qsizetype(position) * m_heads + head]);
                    float *target = sequenceContext + qsizetype(position) * m_dimension + offset;
                    const float *result = attended.constData() + qsizetype(row) * m_headDimension;
                    for (int index = 0; index < m_headDimension; ++index)
                        target[index] = result[index] * gate;
                }
            }
        }
    }

    m_out.apply(context.constData(), normalized.data(), rows);
    const qsizetype total = qsizetype(rows) * m_dimension;
    for (qsizetype index = 0; index < total; ++index)
        values[index] += normalized.at(index);
}

}
