#pragma once

#include <QColor>

namespace theme {

struct Hct
{
    double hue = 0.0;
    double chroma = 0.0;
    double tone = 0.0;

    static Hct fromColor(const QColor &color);
    static QColor toColor(double hue, double chroma, double tone);
    static QColor gray(double tone);
};

}
