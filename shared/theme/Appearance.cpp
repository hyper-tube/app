#include "Appearance.h"

#include "SystemTheme.h"

#include <QCoreApplication>
#include <QSettings>

namespace {

const QString kModeKey = QStringLiteral("appearance/mode");
const QString kSeedKey = QStringLiteral("appearance/seed");
const QString kDefaultSeed = QStringLiteral("#7C83FF");

theme::Appearance::Mode modeFrom(const QString &stored)
{
    if (stored == QLatin1String("light"))
        return theme::Appearance::Light;
    if (stored == QLatin1String("dark"))
        return theme::Appearance::Dark;
    return theme::Appearance::System;
}

QString nameOf(theme::Appearance::Mode mode)
{
    switch (mode) {
    case theme::Appearance::Light: return QStringLiteral("light");
    case theme::Appearance::Dark: return QStringLiteral("dark");
    case theme::Appearance::System: break;
    }
    return QStringLiteral("system");
}

}

namespace theme {

Appearance::Appearance(QObject *parent)
    : QObject(parent)
{
    const QSettings settings;
    m_mode = modeFrom(settings.value(kModeKey).toString());

    const QColor stored(settings.value(kSeedKey).toString());
    if (stored.isValid())
        m_customSeed = stored;

    connect(&SystemTheme::instance(), &SystemTheme::changed, this, &Appearance::changed);
}

Appearance &Appearance::instance()
{
    static auto *appearance = new Appearance(QCoreApplication::instance());
    return *appearance;
}

Appearance *Appearance::create(QQmlEngine *, QJSEngine *)
{
    QQmlEngine::setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
    return &instance();
}

QColor Appearance::seed() const
{
    if (m_customSeed.isValid())
        return m_customSeed;

    const QColor system = SystemTheme::instance().accent();
    return system.isValid() ? system : QColor(kDefaultSeed);
}

bool Appearance::followsSystemColor() const
{
    return !m_customSeed.isValid() && systemColorAvailable();
}

bool Appearance::systemColorAvailable() const
{
    return SystemTheme::instance().available();
}

bool Appearance::dark() const
{
    switch (m_mode) {
    case Light: return false;
    case Dark: return true;
    case System: break;
    }
    return SystemTheme::instance().dark();
}

bool Appearance::generates() const
{
    const SystemTheme &system = SystemTheme::instance();
    return m_customSeed.isValid() || !system.complete() || dark() != system.dark();
}

void Appearance::setMode(Mode mode)
{
    if (m_mode == mode)
        return;
    m_mode = mode;
    save();
    Q_EMIT changed();
}

void Appearance::setSeed(const QColor &seed)
{
    if (m_customSeed == seed || !seed.isValid())
        return;
    m_customSeed = seed;
    save();
    Q_EMIT changed();
}

void Appearance::followSystemColor()
{
    if (!m_customSeed.isValid())
        return;
    m_customSeed = QColor();
    save();
    Q_EMIT changed();
}

void Appearance::save() const
{
    QSettings settings;
    settings.setValue(kModeKey, nameOf(m_mode));
    settings.setValue(kSeedKey,
                      m_customSeed.isValid() ? m_customSeed.name(QColor::HexRgb) : QString());
}

}
