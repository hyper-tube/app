#pragma once

namespace analysis {

class Gemm
{
public:
    static void multiply(const float *left, const float *right, float *result, int rows, int inner,
                         int columns);
    static void multiplyBiased(const float *left, const float *right, const float *bias,
                               float *result, int rows, int inner, int columns);
};

}
