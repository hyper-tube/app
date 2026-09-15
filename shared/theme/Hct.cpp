#include "Hct.h"

#include <QtMath>

#include <array>
#include <cmath>
#include <optional>

namespace {

using Triplet = std::array<double, 3>;

constexpr double kEpsilon = 216.0 / 24389.0;
constexpr double kKappa = 24389.0 / 27.0;
constexpr double kChromaResolution = 0.4;
constexpr double kGamutTolerance = 0.05;
constexpr int kToneIterations = 24;

double labForward(double value)
{
    return value > kEpsilon ? std::cbrt(value) : (kKappa * value + 16.0) / 116.0;
}

double labInverse(double value)
{
    const double cubed = value * value * value;
    return cubed > kEpsilon ? cubed : (116.0 * value - 16.0) / kKappa;
}

double luminanceFromTone(double tone)
{
    return 100.0 * labInverse((tone + 16.0) / 116.0);
}

double toneFromLuminance(double luminance)
{
    return 116.0 * labForward(luminance / 100.0) - 16.0;
}

double linearize(int component)
{
    const double normalized = component / 255.0;
    const double linear = normalized <= 0.040449936 ? normalized / 12.92
                                                    : std::pow((normalized + 0.055) / 1.055, 2.4);
    return linear * 100.0;
}

int delinearize(double component)
{
    const double normalized = component / 100.0;
    const double encoded = normalized <= 0.0031308
        ? normalized * 12.92
        : 1.055 * std::pow(normalized, 1.0 / 2.4) - 0.055;
    return qBound(0, int(std::lround(encoded * 255.0)), 255);
}

Triplet xyzFromLinear(const Triplet &linear)
{
    return {
        0.41233895 * linear[0] + 0.35762064 * linear[1] + 0.18051042 * linear[2],
        0.2126 * linear[0] + 0.7152 * linear[1] + 0.0722 * linear[2],
        0.01932141 * linear[0] + 0.11916382 * linear[1] + 0.95034478 * linear[2],
    };
}

Triplet linearFromXyz(const Triplet &xyz)
{
    return {
        3.2413774792388685 * xyz[0] - 1.5376652402851851 * xyz[1] - 0.49885366846268053 * xyz[2],
        -0.9691452513005321 * xyz[0] + 1.8758853451067872 * xyz[1] + 0.04156585616912061 * xyz[2],
        0.05562093689691305 * xyz[0] - 0.20395524564742123 * xyz[1] + 1.0571799111220335 * xyz[2],
    };
}

double signOf(double value)
{
    return value < 0.0 ? -1.0 : 1.0;
}

double normalizedDegrees(double degrees)
{
    double wrapped = std::fmod(degrees, 360.0);
    if (wrapped < 0.0)
        wrapped += 360.0;
    return wrapped;
}

struct ViewingConditions
{
    Triplet adaptation {};
    double aw = 0.0;
    double nbb = 0.0;
    double ncb = 0.0;
    double c = 0.0;
    double nc = 0.0;
    double n = 0.0;
    double fl = 0.0;
    double flRoot = 0.0;
    double z = 0.0;

    static const ViewingConditions &standard();
};

ViewingConditions makeStandard()
{
    constexpr Triplet white {95.047, 100.0, 108.883};
    const double adaptingLuminance = (200.0 / M_PI) * luminanceFromTone(50.0) / 100.0;

    const double rW = white[0] * 0.401288 + white[1] * 0.650173 + white[2] * -0.051461;
    const double gW = white[0] * -0.250268 + white[1] * 1.204414 + white[2] * 0.045854;
    const double bW = white[0] * -0.002079 + white[1] * 0.048952 + white[2] * 0.953127;

    const double f = 0.8 + 2.0 / 10.0;
    const double c = f >= 0.9 ? 0.59 + (0.69 - 0.59) * ((f - 0.9) * 10.0)
                              : 0.525 + (0.59 - 0.525) * ((f - 0.8) * 10.0);
    const double d =
        qBound(0.0, f * (1.0 - (1.0 / 3.6) * std::exp((-adaptingLuminance - 42.0) / 92.0)), 1.0);

    const Triplet adaptation {
        d * (100.0 / rW) + 1.0 - d,
        d * (100.0 / gW) + 1.0 - d,
        d * (100.0 / bW) + 1.0 - d,
    };

    const double k = 1.0 / (5.0 * adaptingLuminance + 1.0);
    const double k4 = k * k * k * k;
    const double k4Complement = 1.0 - k4;
    const double fl = k4 * adaptingLuminance
        + 0.1 * k4Complement * k4Complement * std::cbrt(5.0 * adaptingLuminance);
    const double n = luminanceFromTone(50.0) / white[1];
    const double nbb = 0.725 / std::pow(n, 0.2);

    const Triplet cone {
        std::pow(fl * adaptation[0] * rW / 100.0, 0.42),
        std::pow(fl * adaptation[1] * gW / 100.0, 0.42),
        std::pow(fl * adaptation[2] * bW / 100.0, 0.42),
    };
    const Triplet adapted {
        400.0 * cone[0] / (cone[0] + 27.13),
        400.0 * cone[1] / (cone[1] + 27.13),
        400.0 * cone[2] / (cone[2] + 27.13),
    };

    ViewingConditions conditions;
    conditions.adaptation = adaptation;
    conditions.aw = (40.0 * adapted[0] + 20.0 * adapted[1] + adapted[2]) / 20.0 * nbb;
    conditions.nbb = nbb;
    conditions.ncb = nbb;
    conditions.c = c;
    conditions.nc = f;
    conditions.n = n;
    conditions.fl = fl;
    conditions.flRoot = std::pow(fl, 0.25);
    conditions.z = 1.48 + std::sqrt(n);
    return conditions;
}

const ViewingConditions &ViewingConditions::standard()
{
    static const ViewingConditions conditions = makeStandard();
    return conditions;
}

struct Appearance
{
    double lightness = 0.0;
    double chroma = 0.0;
    double hue = 0.0;
};

Appearance appearanceOf(const Triplet &xyz)
{
    const ViewingConditions &view = ViewingConditions::standard();

    const double rC = 0.401288 * xyz[0] + 0.650173 * xyz[1] - 0.051461 * xyz[2];
    const double gC = -0.250268 * xyz[0] + 1.204414 * xyz[1] + 0.045854 * xyz[2];
    const double bC = -0.002079 * xyz[0] + 0.048952 * xyz[1] + 0.953127 * xyz[2];

    const Triplet discounted {
        view.adaptation[0] * rC,
        view.adaptation[1] * gC,
        view.adaptation[2] * bC,
    };

    const auto adapt = [&view](double value) {
        const double cone = std::pow(view.fl * std::abs(value) / 100.0, 0.42);
        return signOf(value) * 400.0 * cone / (cone + 27.13);
    };

    const double rA = adapt(discounted[0]);
    const double gA = adapt(discounted[1]);
    const double bA = adapt(discounted[2]);

    const double a = (11.0 * rA - 12.0 * gA + bA) / 11.0;
    const double b = (rA + gA - 2.0 * bA) / 9.0;
    const double u = (20.0 * rA + 20.0 * gA + 21.0 * bA) / 20.0;
    const double p2 = (40.0 * rA + 20.0 * gA + bA) / 20.0;

    const double hue = normalizedDegrees(qRadiansToDegrees(std::atan2(b, a)));
    const double huePrime = hue < 20.14 ? hue + 360.0 : hue;
    const double eccentricity = 0.25 * (std::cos(qDegreesToRadians(huePrime) + 2.0) + 3.8);

    const double achromatic = p2 * view.nbb;
    const double lightness = 100.0 * std::pow(achromatic / view.aw, view.c * view.z);
    const double p1 = 50000.0 / 13.0 * eccentricity * view.nc * view.ncb;
    const double t = p1 * std::hypot(a, b) / (u + 0.305);
    const double alpha = std::pow(t, 0.9) * std::pow(1.64 - std::pow(0.29, view.n), 0.73);

    return {lightness, alpha * std::sqrt(lightness / 100.0), hue};
}

Triplet xyzOf(double lightness, double chroma, double hue)
{
    const ViewingConditions &view = ViewingConditions::standard();

    const double alpha = lightness <= 0.0 ? 0.0 : chroma / std::sqrt(lightness / 100.0);
    const double t = std::pow(alpha / std::pow(1.64 - std::pow(0.29, view.n), 0.73), 1.0 / 0.9);
    const double radians = qDegreesToRadians(hue);
    const double eccentricity = 0.25 * (std::cos(radians + 2.0) + 3.8);

    const double achromatic = view.aw * std::pow(lightness / 100.0, 1.0 / view.c / view.z);
    const double p1 = eccentricity * (50000.0 / 13.0) * view.nc * view.ncb;
    const double p2 = achromatic / view.nbb;

    const double hueSin = std::sin(radians);
    const double hueCos = std::cos(radians);
    const double gamma =
        23.0 * (p2 + 0.305) * t / (23.0 * p1 + 11.0 * t * hueCos + 108.0 * t * hueSin);
    const double a = gamma * hueCos;
    const double b = gamma * hueSin;

    const double rA = (460.0 * p2 + 451.0 * a + 288.0 * b) / 1403.0;
    const double gA = (460.0 * p2 - 891.0 * a - 261.0 * b) / 1403.0;
    const double bA = (460.0 * p2 - 220.0 * a - 6300.0 * b) / 1403.0;

    const auto unadapt = [&view](double value) {
        const double base = qMax(0.0, 27.13 * std::abs(value) / (400.0 - std::abs(value)));
        return signOf(value) * (100.0 / view.fl) * std::pow(base, 1.0 / 0.42);
    };

    const double r = unadapt(rA) / view.adaptation[0];
    const double g = unadapt(gA) / view.adaptation[1];
    const double c = unadapt(bA) / view.adaptation[2];

    return {
        1.86206786 * r - 1.01125463 * g + 0.14918677 * c,
        0.38752654 * r + 0.62144744 * g - 0.00897398 * c,
        -0.01584150 * r - 0.03412294 * g + 1.04996444 * c,
    };
}

std::optional<QColor> solveExact(double hue, double chroma, double tone)
{
    double low = 0.0;
    double high = 100.0;
    Triplet xyz {};

    for (int step = 0; step < kToneIterations; ++step) {
        const double middle = (low + high) / 2.0;
        xyz = xyzOf(qMax(middle, 0.01), chroma, hue);
        if (toneFromLuminance(xyz[1]) < tone)
            low = middle;
        else
            high = middle;
    }

    const Triplet linear = linearFromXyz(xyz);
    for (double const component : linear) {
        if (component < -kGamutTolerance || component > 100.0 + kGamutTolerance)
            return std::nullopt;
    }
    return QColor(delinearize(linear[0]), delinearize(linear[1]), delinearize(linear[2]));
}

}

namespace theme {

Hct Hct::fromColor(const QColor &color)
{
    const QColor rgb = color.toRgb();
    const Triplet linear {
        linearize(rgb.red()),
        linearize(rgb.green()),
        linearize(rgb.blue()),
    };
    const Triplet xyz = xyzFromLinear(linear);
    const Appearance appearance = appearanceOf(xyz);
    return {appearance.hue, appearance.chroma, toneFromLuminance(xyz[1])};
}

QColor Hct::gray(double tone)
{
    const int level = delinearize(luminanceFromTone(qBound(0.0, tone, 100.0)));
    return QColor(level, level, level);
}

QColor Hct::toColor(double hue, double chroma, double tone)
{
    const double bounded = qBound(0.0, tone, 100.0);
    if (chroma < 1.0 || bounded < 1.0 || bounded > 99.0)
        return gray(bounded);

    QColor answer = gray(bounded);
    double low = 0.0;
    double high = chroma;

    while (high - low > kChromaResolution) {
        const double middle = (low + high) / 2.0;
        if (const std::optional<QColor> candidate = solveExact(hue, middle, bounded)) {
            answer = *candidate;
            low = middle;
        } else {
            high = middle;
        }
    }
    return answer;
}

}
