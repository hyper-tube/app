#include "ColorUtils.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr qreal kDarkThreshold = 0.4;

qreal clamp01(qreal value)
{
    return std::clamp(value, qreal(0), qreal(1));
}

qreal linearize(qreal channel)
{
    return channel <= 0.03928 ? channel / 12.92 : std::pow((channel + 0.055) / 1.055, 2.4);
}

}

namespace theme {

ColorUtils::ColorUtils(QObject *parent)
    : QObject(parent)
{
}

QColor ColorUtils::mix(const QColor &a, const QColor &b, qreal ratio)
{
    const qreal t = clamp01(ratio);
    return QColor::fromRgbF(
        t * a.redF() + (1 - t) * b.redF(), t * a.greenF() + (1 - t) * b.greenF(),
        t * a.blueF() + (1 - t) * b.blueF(), t * a.alphaF() + (1 - t) * b.alphaF());
}

QColor ColorUtils::withAlpha(const QColor &source, qreal alpha)
{
    return QColor::fromRgbF(source.redF(), source.greenF(), source.blueF(), clamp01(alpha));
}

QColor ColorUtils::transparentize(const QColor &source, qreal amount)
{
    return withAlpha(source, source.alphaF() * (1 - clamp01(amount)));
}

QColor ColorUtils::layer(const QColor &base, const QColor &overlay, qreal alpha)
{
    const qreal t = clamp01(alpha);
    return QColor::fromRgbF(overlay.redF() * t + base.redF() * (1 - t),
                            overlay.greenF() * t + base.greenF() * (1 - t),
                            overlay.blueF() * t + base.blueF() * (1 - t), 1);
}

qreal ColorUtils::luminance(const QColor &source)
{
    return 0.2126 * linearize(source.redF()) + 0.7152 * linearize(source.greenF())
        + 0.0722 * linearize(source.blueF());
}

bool ColorUtils::isDark(const QColor &source)
{
    return luminance(source) < kDarkThreshold;
}

QColor ColorUtils::contrasting(const QColor &background, const QColor &light, const QColor &dark)
{
    return isDark(background) ? light : dark;
}

}
