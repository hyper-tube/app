#include "Palette.h"

#include "Appearance.h"
#include "ColorUtils.h"
#include "Scheme.h"
#include "SystemTheme.h"

#include <QGuiApplication>
#include <QHash>
#include <QStyleHints>

namespace {

struct Role
{
    const char *key;
    const char *fallback;
    QColor theme::Palette::*member;
};

}

namespace theme {

Palette::Palette(QObject *parent)
    : QObject(parent)
{
    connect(&Appearance::instance(), &Appearance::changed, this, &Palette::reload);
    reload();
}

bool Palette::darkMode() const
{
    return ColorUtils::isDark(m_background);
}

void Palette::reload()
{
    static const Role kRoles[] = {
        {"background", "#12101C", &Palette::m_background},
        {"error", "#FFB4AB", &Palette::m_error},
        {"error_container", "#93000A", &Palette::m_errorContainer},
        {"inverse_on_surface", "#303036", &Palette::m_inverseOnSurface},
        {"inverse_primary", "#5F63C5", &Palette::m_inversePrimary},
        {"inverse_surface", "#E4E1E9", &Palette::m_inverseSurface},
        {"on_background", "#E7E0F0", &Palette::m_onBackground},
        {"on_error", "#690005", &Palette::m_onError},
        {"on_error_container", "#FFDAD6", &Palette::m_onErrorContainer},
        {"on_primary", "#111027", &Palette::m_onPrimary},
        {"on_primary_container", "#E3DEFF", &Palette::m_onPrimaryContainer},
        {"on_primary_fixed", "#11144B", &Palette::m_onPrimaryFixed},
        {"on_primary_fixed_variant", "#3E4178", &Palette::m_onPrimaryFixedVariant},
        {"on_secondary", "#2E2F42", &Palette::m_onSecondary},
        {"on_secondary_container", "#E7E0FF", &Palette::m_onSecondaryContainer},
        {"on_secondary_fixed", "#191A2C", &Palette::m_onSecondaryFixed},
        {"on_secondary_fixed_variant", "#454559", &Palette::m_onSecondaryFixedVariant},
        {"on_surface", "#E7E0F0", &Palette::m_onSurface},
        {"on_surface_variant", "#CAC4D0", &Palette::m_onSurfaceVariant},
        {"on_tertiary", "#3B2948", &Palette::m_onTertiary},
        {"on_tertiary_container", "#F0DBFF", &Palette::m_onTertiaryContainer},
        {"on_tertiary_fixed", "#26102F", &Palette::m_onTertiaryFixed},
        {"on_tertiary_fixed_variant", "#523F5F", &Palette::m_onTertiaryFixedVariant},
        {"outline", "#9A91B4", &Palette::m_outline},
        {"outline_variant", "#46464F", &Palette::m_outlineVariant},
        {"primary", "#7C83FF", &Palette::m_primary},
        {"primary_container", "#332B67", &Palette::m_primaryContainer},
        {"primary_fixed", "#E3DEFF", &Palette::m_primaryFixed},
        {"primary_fixed_dim", "#BFC2FF", &Palette::m_primaryFixedDim},
        {"scrim", "#000000", &Palette::m_scrim},
        {"secondary", "#BDB5E9", &Palette::m_secondary},
        {"secondary_container", "#413B60", &Palette::m_secondaryContainer},
        {"secondary_fixed", "#E7E0FF", &Palette::m_secondaryFixed},
        {"secondary_fixed_dim", "#C7C2DD", &Palette::m_secondaryFixedDim},
        {"shadow", "#000000", &Palette::m_shadow},
        {"surface", "#12101C", &Palette::m_surface},
        {"surface_bright", "#3A354A", &Palette::m_surfaceBright},
        {"surface_container", "#1D192C", &Palette::m_surfaceContainer},
        {"surface_container_high", "#28223A", &Palette::m_surfaceContainerHigh},
        {"surface_container_highest", "#342D48", &Palette::m_surfaceContainerHighest},
        {"surface_container_low", "#181426", &Palette::m_surfaceContainerLow},
        {"surface_container_lowest", "#0C0A14", &Palette::m_surfaceContainerLowest},
        {"surface_dim", "#12101C", &Palette::m_surfaceDim},
        {"surface_tint", "#7C83FF", &Palette::m_surfaceTint},
        {"surface_variant", "#46464F", &Palette::m_surfaceVariant},
        {"tertiary", "#C59DEB", &Palette::m_tertiary},
        {"tertiary_container", "#553A68", &Palette::m_tertiaryContainer},
        {"tertiary_fixed", "#F0DBFF", &Palette::m_tertiaryFixed},
        {"tertiary_fixed_dim", "#D8BBEE", &Palette::m_tertiaryFixedDim},
    };

    const Appearance &appearance = Appearance::instance();
    const ColorSource::Roles &provided = SystemTheme::instance().roles();

    QHash<QString, QColor> generated;
    if (appearance.generates())
        generated = schemeFrom(appearance.seed(), appearance.dark());

    for (const Role &role : kRoles) {
        const QString key = QString::fromLatin1(role.key);
        const QColor chosen = generated.value(key, provided.value(key));
        this->*role.member = chosen.isValid() ? chosen : QColor(QLatin1String(role.fallback));
    }

    QGuiApplication::styleHints()->setColorScheme(darkMode() ? Qt::ColorScheme::Dark
                                                             : Qt::ColorScheme::Light);
    Q_EMIT changed();
}

}
