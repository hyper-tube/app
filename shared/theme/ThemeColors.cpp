#include "ThemeColors.h"

#include "ActiveTheme.h"
#include "Scheme.h"

namespace {

const QString kSourceId = QStringLiteral("theme");
const QString kPrimaryRole = QStringLiteral("primary");

theme::ColorSource::Roles completed(const theme::ColorSource::Roles &given, const QColor &accent,
                                    bool dark)
{
    theme::ColorSource::Roles roles = theme::schemeFrom(accent, dark);
    for (auto role = given.constBegin(); role != given.constEnd(); ++role)
        roles.insert(role.key(), role.value());
    return roles;
}

QColor accentOf(const theme::ThemeScheme &scheme)
{
    if (scheme.seed.isValid())
        return scheme.seed;
    const QColor dark = scheme.dark.value(kPrimaryRole);
    return dark.isValid() ? dark : scheme.light.value(kPrimaryRole);
}

Qt::ColorScheme colorSchemeOf(const theme::ThemeScheme &scheme)
{
    if (scheme.light.isEmpty() == scheme.dark.isEmpty())
        return Qt::ColorScheme::Unknown;
    return scheme.dark.isEmpty() ? Qt::ColorScheme::Light : Qt::ColorScheme::Dark;
}

}

namespace theme {

ThemeColors::ThemeColors(QObject *parent)
    : ColorSource(kSourceId, QString(), QString(), parent)
{
    connect(&ActiveTheme::instance(), &ActiveTheme::schemeChanged, this, &ThemeColors::refresh);
}

QString ThemeColors::sourceId()
{
    return kSourceId;
}

void ThemeColors::refresh()
{
    const ThemeScheme &scheme = ActiveTheme::instance().scheme();
    relabel(scheme.name, scheme.location);

    const QColor accent = accentOf(scheme);
    if (!scheme.carriesPalette() || !accent.isValid()) {
        clear();
        return;
    }

    if (scheme.light.isEmpty() && scheme.dark.isEmpty()) {
        adopt({}, {}, accent, Qt::ColorScheme::Unknown);
        return;
    }

    adopt(completed(scheme.light, accent, false), completed(scheme.dark, accent, true), accent,
          colorSchemeOf(scheme));
}

}
