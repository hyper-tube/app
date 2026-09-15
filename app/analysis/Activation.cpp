#include "Activation.h"

#include <cmath>
#include <numbers>

namespace {

constexpr float kInverseRootTwo = 0.70710678118654752f;

}

namespace analysis {

void Activation::gelu(float *values, qsizetype count)
{
    for (qsizetype index = 0; index < count; ++index) {
        const float value = values[index];
        values[index] = 0.5f * value * (1.0f + std::erf(value * kInverseRootTwo));
    }
}

}
