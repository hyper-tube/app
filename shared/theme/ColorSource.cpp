#include "ColorSource.h"

#include "ColorUtils.h"

#include <utility>

namespace theme {

ColorSource::ColorSource(QString id, QString name, QString location, QObject *parent)
    : QObject(parent)
    , m_id(std::move(id))
    , m_name(std::move(name))
    , m_location(std::move(location))
{
}

ColorSourceInfo ColorSource::info(bool active) const
{
    return {m_id, m_name, m_location, active, complete()};
}

Qt::ColorScheme ColorSource::schemeFor(const QColor &background)
{
    if (!background.isValid())
        return Qt::ColorScheme::Unknown;
    return ColorUtils::isDark(background) ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light;
}

void ColorSource::adopt(const Roles &roles, const QColor &accent, Qt::ColorScheme colorScheme)
{
    if (colorScheme == Qt::ColorScheme::Light)
        adopt(roles, {}, accent, colorScheme);
    else
        adopt({}, roles, accent, colorScheme);
}

void ColorSource::adopt(const Roles &light, const Roles &dark, const QColor &accent,
                        Qt::ColorScheme colorScheme)
{
    if (m_light == light && m_dark == dark && m_accent == accent && m_colorScheme == colorScheme)
        return;
    m_light = light;
    m_dark = dark;
    m_accent = accent;
    m_colorScheme = colorScheme;
    Q_EMIT changed();
}

void ColorSource::relabel(const QString &name, const QString &location)
{
    if (m_name == name && m_location == location)
        return;
    m_name = name;
    m_location = location;
    Q_EMIT changed();
}

void ColorSource::clear()
{
    adopt({}, {}, QColor(), Qt::ColorScheme::Unknown);
}

}
