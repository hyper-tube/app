#pragma once

#include <QColor>
#include <QHash>
#include <QString>

namespace theme {

struct ThemeScheme
{
    using Roles = QHash<QString, QColor>;

    QString name;
    QString location;
    QColor seed;
    Roles light;
    Roles dark;
    Roles lightOverrides;
    Roles darkOverrides;

    bool carriesPalette() const { return seed.isValid() || !light.isEmpty() || !dark.isEmpty(); }
    const Roles &overrides(bool darkMode) const
    {
        return darkMode ? darkOverrides : lightOverrides;
    }

    bool operator==(const ThemeScheme &) const = default;
};

}
