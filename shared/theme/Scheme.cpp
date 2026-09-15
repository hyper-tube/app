#include "Scheme.h"

#include "Hct.h"

namespace {

class TonalPalette
{
public:
    TonalPalette(double hue, double chroma)
        : m_hue(hue)
        , m_chroma(chroma)
    {
    }

    QColor tone(double level) const { return theme::Hct::toColor(m_hue, m_chroma, level); }

private:
    double m_hue;
    double m_chroma;
};

struct Palettes
{
    TonalPalette primary;
    TonalPalette secondary;
    TonalPalette tertiary;
    TonalPalette neutral;
    TonalPalette neutralVariant;
    TonalPalette error;
};

Palettes palettesFor(const QColor &seed)
{
    const theme::Hct source = theme::Hct::fromColor(seed);
    return {
        TonalPalette(source.hue, 36.0),        TonalPalette(source.hue, 16.0),
        TonalPalette(source.hue + 60.0, 24.0), TonalPalette(source.hue, 6.0),
        TonalPalette(source.hue, 8.0),         TonalPalette(25.0, 84.0),
    };
}

void insertAccent(QHash<QString, QColor> &roles, const QString &name, const TonalPalette &palette,
                  bool dark)
{
    roles.insert(name, palette.tone(dark ? 80 : 40));
    roles.insert(QStringLiteral("on_") + name, palette.tone(dark ? 20 : 100));
    roles.insert(name + QStringLiteral("_container"), palette.tone(dark ? 30 : 90));
    roles.insert(QStringLiteral("on_") + name + QStringLiteral("_container"),
                 palette.tone(dark ? 90 : 10));
}

void insertFixed(QHash<QString, QColor> &roles, const QString &name, const TonalPalette &palette)
{
    roles.insert(name + QStringLiteral("_fixed"), palette.tone(90));
    roles.insert(name + QStringLiteral("_fixed_dim"), palette.tone(80));
    roles.insert(QStringLiteral("on_") + name + QStringLiteral("_fixed"), palette.tone(10));
    roles.insert(QStringLiteral("on_") + name + QStringLiteral("_fixed_variant"), palette.tone(30));
}

}

namespace theme {

QHash<QString, QColor> schemeFrom(const QColor &seed, bool dark)
{
    const Palettes palettes = palettesFor(seed);
    QHash<QString, QColor> roles;

    insertAccent(roles, QStringLiteral("primary"), palettes.primary, dark);
    insertAccent(roles, QStringLiteral("secondary"), palettes.secondary, dark);
    insertAccent(roles, QStringLiteral("tertiary"), palettes.tertiary, dark);
    insertAccent(roles, QStringLiteral("error"), palettes.error, dark);

    insertFixed(roles, QStringLiteral("primary"), palettes.primary);
    insertFixed(roles, QStringLiteral("secondary"), palettes.secondary);
    insertFixed(roles, QStringLiteral("tertiary"), palettes.tertiary);

    roles.insert(QStringLiteral("background"), palettes.neutral.tone(dark ? 6 : 98));
    roles.insert(QStringLiteral("on_background"), palettes.neutral.tone(dark ? 90 : 10));
    roles.insert(QStringLiteral("surface"), palettes.neutral.tone(dark ? 6 : 98));
    roles.insert(QStringLiteral("on_surface"), palettes.neutral.tone(dark ? 90 : 10));
    roles.insert(QStringLiteral("surface_dim"), palettes.neutral.tone(dark ? 6 : 87));
    roles.insert(QStringLiteral("surface_bright"), palettes.neutral.tone(dark ? 24 : 98));
    roles.insert(QStringLiteral("surface_container_lowest"), palettes.neutral.tone(dark ? 4 : 100));
    roles.insert(QStringLiteral("surface_container_low"), palettes.neutral.tone(dark ? 10 : 96));
    roles.insert(QStringLiteral("surface_container"), palettes.neutral.tone(dark ? 12 : 94));
    roles.insert(QStringLiteral("surface_container_high"), palettes.neutral.tone(dark ? 17 : 92));
    roles.insert(QStringLiteral("surface_container_highest"),
                 palettes.neutral.tone(dark ? 22 : 90));
    roles.insert(QStringLiteral("surface_variant"), palettes.neutralVariant.tone(dark ? 30 : 90));
    roles.insert(QStringLiteral("on_surface_variant"),
                 palettes.neutralVariant.tone(dark ? 80 : 30));
    roles.insert(QStringLiteral("outline"), palettes.neutralVariant.tone(dark ? 60 : 50));
    roles.insert(QStringLiteral("outline_variant"), palettes.neutralVariant.tone(dark ? 30 : 80));
    roles.insert(QStringLiteral("inverse_surface"), palettes.neutral.tone(dark ? 90 : 20));
    roles.insert(QStringLiteral("inverse_on_surface"), palettes.neutral.tone(dark ? 20 : 95));
    roles.insert(QStringLiteral("inverse_primary"), palettes.primary.tone(dark ? 40 : 80));
    roles.insert(QStringLiteral("surface_tint"), palettes.primary.tone(dark ? 80 : 40));
    roles.insert(QStringLiteral("shadow"), palettes.neutral.tone(0));
    roles.insert(QStringLiteral("scrim"), palettes.neutral.tone(0));

    return roles;
}

}
