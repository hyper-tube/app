#include "SystemTheme.h"

#include "core/Logging.h"
#include "MaterialColors.h"
#include "PlasmaColors.h"
#include "PlatformColors.h"
#include "TerminalColors.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QSettings>
#include <QStandardPaths>
#include <QStyleHints>

namespace {

const QString kSourceKey = QStringLiteral("appearance/colorSource");

struct FileSource
{
    QString id;
    QString name;
    QString path;
};

QString underHome(QStandardPaths::StandardLocation location, const QString &relative)
{
    const QString root = QStandardPaths::writableLocation(location);
    return root.isEmpty() ? QString() : root + QLatin1Char('/') + relative;
}

bool desktopShellsRunHere()
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    return true;
#else
    return false;
#endif
}

QList<FileSource> materialFiles()
{
    if (!desktopShellsRunHere())
        return {};
    return {
        {QStringLiteral("quickshell"), QStringLiteral("Quickshell"),
         underHome(QStandardPaths::GenericStateLocation,
                   QStringLiteral("quickshell/user/generated/colors.json"))},
        {QStringLiteral("matugen-config"), QStringLiteral("Matugen"),
         underHome(QStandardPaths::GenericConfigLocation, QStringLiteral("matugen/colors.json"))},
        {QStringLiteral("matugen-cache"), QStringLiteral("Matugen"),
         underHome(QStandardPaths::GenericCacheLocation, QStringLiteral("matugen/colors.json"))},
    };
}

QList<FileSource> terminalFiles()
{
    if (!desktopShellsRunHere())
        return {};
    return {
        {QStringLiteral("wallust"), QStringLiteral("Wallust"),
         underHome(QStandardPaths::GenericCacheLocation, QStringLiteral("wallust/colors.json"))},
        {QStringLiteral("pywal"), QStringLiteral("Pywal"),
         underHome(QStandardPaths::GenericCacheLocation, QStringLiteral("wal/colors.json"))},
    };
}

QString plasmaFile()
{
    if (!desktopShellsRunHere())
        return {};
    return underHome(QStandardPaths::GenericConfigLocation, QStringLiteral("kdeglobals"));
}

}

namespace theme {

SystemTheme::SystemTheme(QObject *parent)
    : QObject(parent)
    , m_platformScheme(QGuiApplication::styleHints()->colorScheme())
    , m_preferredId(QSettings().value(kSourceKey).toString())
{
    discover();

    if (const ColorSource *source = active())
        qCInfo(logTheme) << "system colors from" << source->name() << source->location();
    else
        qCInfo(logTheme) << "no system color source, using the built-in palette";
}

SystemTheme &SystemTheme::instance()
{
    static auto *systemTheme = new SystemTheme(QCoreApplication::instance());
    return *systemTheme;
}

SystemTheme *SystemTheme::create(QQmlEngine *, QJSEngine *)
{
    QQmlEngine::setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
    return &instance();
}

bool SystemTheme::complete() const
{
    const ColorSource *source = active();
    return source && source->complete();
}

bool SystemTheme::dark() const
{
    return colorScheme() != Qt::ColorScheme::Light;
}

QString SystemTheme::sourceName() const
{
    const ColorSource *source = active();
    return source ? source->name() : QString();
}

QList<ColorSourceInfo> SystemTheme::sources() const
{
    const ColorSource *current = active();

    QList<ColorSourceInfo> found;
    for (const ColorSource *source : m_sources) {
        if (source->available())
            found.append(source->info(source == current));
    }
    return found;
}

const ColorSource::Roles &SystemTheme::roles() const
{
    static const ColorSource::Roles kNone;
    const ColorSource *source = active();
    return source ? source->roles() : kNone;
}

QColor SystemTheme::accent() const
{
    const ColorSource *source = active();
    return source ? source->accent() : QColor();
}

void SystemTheme::select(const QString &id)
{
    if (m_preferredId == id)
        return;
    m_preferredId = id;
    QSettings().setValue(kSourceKey, id);
    Q_EMIT changed();
}

void SystemTheme::discover()
{
    for (const FileSource &file : materialFiles()) {
        if (!file.path.isEmpty())
            m_sources.append(new MaterialColors(file.id, file.name, file.path, this));
    }

    m_sources.append(new PlatformColors(this));

    if (const QString plasma = plasmaFile(); !plasma.isEmpty())
        m_sources.append(new PlasmaColors(plasma, this));

    for (const FileSource &file : terminalFiles()) {
        if (!file.path.isEmpty())
            m_sources.append(new TerminalColors(file.id, file.name, file.path, this));
    }

    for (ColorSource *source : std::as_const(m_sources)) {
        connect(source, &ColorSource::changed, this, &SystemTheme::changed);
        source->refresh();
    }
}

const ColorSource *SystemTheme::active() const
{
    for (const ColorSource *source : m_sources) {
        if (source->available() && source->id() == m_preferredId)
            return source;
    }
    for (const ColorSource *source : m_sources) {
        if (source->available())
            return source;
    }
    return nullptr;
}

Qt::ColorScheme SystemTheme::colorScheme() const
{
    if (const ColorSource *source = active();
        source && source->colorScheme() != Qt::ColorScheme::Unknown) {
        return source->colorScheme();
    }

    for (const ColorSource *source : m_sources) {
        if (source->colorScheme() != Qt::ColorScheme::Unknown)
            return source->colorScheme();
    }
    return m_platformScheme;
}

}
