#include "Fft.h"

#include <bit>
#include <cmath>
#include <numbers>

namespace {

int reverseBits(int value, int bits)
{
    quint32 source = quint32(value);
    quint32 reversed = 0;
    for (int index = 0; index < bits; ++index) {
        reversed = (reversed << 1u) | (source & 1u);
        source >>= 1u;
    }
    return int(reversed);
}

}

namespace analysis {

Fft::Fft(int size)
    : m_size(size)
{
    if (size <= 0 || !std::has_single_bit(quint32(size))) {
        m_size = 0;
        return;
    }

    const int bits = std::countr_zero(quint32(size));

    m_reversed.resize(size);
    for (int index = 0; index < size; ++index)
        m_reversed[index] = reverseBits(index, bits);

    m_cosine.resize(size / 2);
    m_sine.resize(size / 2);
    for (int index = 0; index < size / 2; ++index) {
        const double angle = 2.0 * std::numbers::pi * index / size;
        m_cosine[index] = float(std::cos(angle));
        m_sine[index] = float(std::sin(angle));
    }
}

void Fft::transform(float *real, float *imaginary) const
{
    if (m_size <= 0)
        return;

    for (int index = 0; index < m_size; ++index) {
        const int target = m_reversed.at(index);
        if (index < target) {
            std::swap(real[index], real[target]);
            std::swap(imaginary[index], imaginary[target]);
        }
    }

    for (int span = 2; span <= m_size; span *= 2) {
        const int half = span / 2;
        const int step = m_size / span;
        for (int base = 0; base < m_size; base += span) {
            for (int offset = 0; offset < half; ++offset) {
                const qsizetype twiddle = qsizetype(offset) * step;
                const float cosine = m_cosine.at(twiddle);
                const float sine = -m_sine.at(twiddle);
                const int even = base + offset;
                const int odd = even + half;
                const float realOdd = real[odd] * cosine - imaginary[odd] * sine;
                const float imaginaryOdd = real[odd] * sine + imaginary[odd] * cosine;
                real[odd] = real[even] - realOdd;
                imaginary[odd] = imaginary[even] - imaginaryOdd;
                real[even] += realOdd;
                imaginary[even] += imaginaryOdd;
            }
        }
    }
}

}
