#pragma once

#include <QtGlobal>

namespace analysis {

class Activation
{
public:
    static void gelu(float *values, qsizetype count);
};

}
