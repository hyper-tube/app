#include "ThemeLibrary.h"

#include "ActiveTheme.h"
#include "Appearance.h"
#include "SystemTheme.h"
#include "ThemeColors.h"
#include "ThemeSchema.h"
#include "Tokens.h"
#include "core/Localization.h"
#include "core/Logging.h"
#include "core/Paths.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QSaveFile>
#include <QSettings>
#include <QUrl>

#include <algorithm>

namespace {

const QString kThemeKey = QStringLiteral("appearance/theme");
const QString kDefaultId = QStringLiteral("default");
const QString kThemeFile = QStringLiteral("theme.json");
const QString kSchemaFile = QStringLiteral("theme.schema.json");
const QString kBuiltInRoot = QStringLiteral(":/themes/");
const QString kPrimaryRole = QStringLiteral("primary");
const QStringList kFamilyTokens = {
    QStringLiteral("typography.family"),
    QStringLiteral("icons.family"),
};
constexpr int kSettleMs = 150;

struct BuiltIn
{
    const char *id;
    const char *name;
    const char *description;
};

const BuiltIn kBuiltIns[] = {
    {"default", QT_TRANSLATE_NOOP("theme::ThemeLibrary", "Default"),
     QT_TRANSLATE_NOOP("theme::ThemeLibrary", "Rounded corners and pill shaped controls")},
    {"square", QT_TRANSLATE_NOOP("theme::ThemeLibrary", "Square"),
     QT_TRANSLATE_NOOP("theme::ThemeLibrary", "Small corners, closer to YouTube Music on the web")},
};

const BuiltIn *builtIn(const QString &id)
{
    for (const BuiltIn &entry : kBuiltIns) {
        if (id == QLatin1String(entry.id))
            return &entry;
    }
    return nullptr;
}

QByteArray contentsOf(const QString &path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

}

namespace theme {

ThemeLibrary::ThemeLibrary(QObject *parent)
    : QObject(parent)
{
    m_settle.setSingleShot(true);
    m_settle.setInterval(kSettleMs);
    connect(&m_settle, &QTimer::timeout, this, &ThemeLibrary::reload);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, &m_settle,
            [this] { m_settle.start(); });
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, &m_settle, [this] { m_settle.start(); });
    connect(&core::Localization::instance(), &core::Localization::resolvedChanged, this,
            &ThemeLibrary::reload);
}

ThemeLibrary &ThemeLibrary::instance()
{
    static auto *library = new ThemeLibrary(QCoreApplication::instance());
    return *library;
}

ThemeLibrary *ThemeLibrary::create(QQmlEngine *, QJSEngine *)
{
    QQmlEngine::setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
    return &instance();
}

QVariantList ThemeLibrary::options() const
{
    QVariantList options;
    for (const ThemeFile &theme : m_themes) {
        options.append(QVariantMap {
            {QStringLiteral("value"), theme.id},
            {QStringLiteral("label"), nameOf(theme)},
            {QStringLiteral("caption"), captionOf(theme)},
        });
    }
    return options;
}

QString ThemeLibrary::caption() const
{
    const ThemeFile *theme = find(m_current);
    return theme ? captionOf(*theme) : QString();
}

QString ThemeLibrary::name() const
{
    const ThemeFile *theme = find(m_current);
    return theme ? nameOf(*theme) : QString();
}

QString ThemeLibrary::folder()
{
    return core::paths::themeDir();
}

void ThemeLibrary::restore()
{
    m_chosen = QSettings().value(kThemeKey, kDefaultId).toString();
    writeSchema();
    reload();
    qCInfo(logTheme) << "theme" << m_current << "of" << m_themes.size() << "found in" << folder();
}

void ThemeLibrary::select(const QString &id)
{
    if ((id == m_chosen && id == m_current) || !find(id))
        return;
    m_chosen = id;
    QSettings().setValue(kThemeKey, id);
    apply();

    if (!ActiveTheme::instance().scheme().carriesPalette())
        return;
    SystemTheme::instance().select(ThemeColors::sourceId());
    Appearance::instance().followSystemColor();
}

void ThemeLibrary::openFolder() const
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(folder()));
}

void ThemeLibrary::reload()
{
    const QVariantList before = options();
    scan();
    watch();
    if (options() != before)
        Q_EMIT listChanged();
    apply();
}

void ThemeLibrary::scan()
{
    QList<ThemeFile> themes;
    for (const BuiltIn &entry : kBuiltIns) {
        const QString id = QString::fromLatin1(entry.id);
        const QString path = kBuiltInRoot + id + QLatin1Char('/') + kThemeFile;
        if (QFile::exists(path)) {
            themes.append(ThemeFile::read(id, path));
        } else {
            ThemeFile theme;
            theme.id = id;
            themes.append(theme);
        }
    }

    QList<ThemeFile> installed;
    const QFileInfoList folders =
        QDir(folder()).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QFileInfo &entry : folders) {
        const QString path = QDir(entry.absoluteFilePath()).filePath(kThemeFile);
        if (!QFileInfo::exists(path))
            continue;
        if (builtIn(entry.fileName())) {
            qCWarning(logTheme) << "theme folder is named after a built-in theme and is skipped"
                                << entry.absoluteFilePath();
            continue;
        }
        installed.append(ThemeFile::read(entry.fileName(), path));
    }
    std::ranges::sort(installed, [](const ThemeFile &a, const ThemeFile &b) {
        return QString::localeAwareCompare(a.name, b.name) < 0;
    });

    m_themes = themes + installed;
}

void ThemeLibrary::watch()
{
    QStringList paths {folder()};
    for (const ThemeFile &theme : std::as_const(m_themes)) {
        if (!builtIn(theme.id))
            paths << theme.directory() << theme.path;
    }

    const QStringList watched = m_watcher.directories() + m_watcher.files();
    for (const QString &path : std::as_const(paths)) {
        if (!watched.contains(path))
            m_watcher.addPath(path);
    }
}

void ThemeLibrary::apply()
{
    const QString previous = m_current;
    const QString previousName = name();
    const QString previousCaption = caption();

    QStringList problems;
    const ThemeFile *leaf = find(m_chosen);
    if (!leaf) {
        problems.append(
            tr("The theme %1 is not installed any more, so Default is used").arg(m_chosen));
        leaf = find(kDefaultId);
    }
    m_current = leaf->id;

    const QList<const ThemeFile *> chain = chainOf(*leaf, problems);
    QVariantHash chosen;
    for (const ThemeFile *theme : chain) {
        for (const QString &problem : theme->problems)
            problems.append(theme == leaf ? problem : tr("%1: %2").arg(nameOf(*theme), problem));
        chosen.insert(theme->values);
    }

    const QHash<QString, int> fonts = load(fontsOf(chain, problems), problems);
    QVariantMap values = tokens::resolve(chosen);
    for (const QString &token : kFamilyTokens) {
        const QString family = values.value(token).toString();
        if (QFontDatabase::hasFamily(family))
            continue;
        problems.append(tr("No font family is called %1. Install it, or list its file under fonts")
                            .arg(family));
        values.insert(token, tokens::defaults().value(token));
    }

    ActiveTheme::instance().apply(values, schemeOf(*leaf, chain, problems));

    for (auto font = m_fonts.constBegin(); font != m_fonts.constEnd(); ++font) {
        if (!fonts.contains(font.key()))
            QFontDatabase::removeApplicationFont(font.value());
    }
    m_fonts = fonts;

    adoptProblems(problems);
    if (m_current != previous || name() != previousName || caption() != previousCaption)
        Q_EMIT currentChanged();
}

void ThemeLibrary::writeSchema() const
{
    const QString path = QDir(folder()).filePath(kSchemaFile);
    const QByteArray schema = themeSchema();
    if (contentsOf(path) == schema)
        return;

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(schema) != schema.size() || !file.commit())
        qCWarning(logTheme) << "theme schema could not be written to" << path;
}

void ThemeLibrary::adoptProblems(const QStringList &problems)
{
    if (m_problems == problems)
        return;
    m_problems = problems;
    for (const QString &problem : problems)
        qCWarning(logTheme) << "theme" << m_current << problem;
    Q_EMIT problemsChanged();
}

const ThemeFile *ThemeLibrary::find(const QString &id) const
{
    const auto found = std::ranges::find(m_themes, id, &ThemeFile::id);
    return found == m_themes.cend() ? nullptr : &*found;
}

QList<const ThemeFile *> ThemeLibrary::chainOf(const ThemeFile &leaf, QStringList &problems) const
{
    QList<const ThemeFile *> chain;
    QStringList seen;
    const ThemeFile *theme = &leaf;
    while (theme) {
        if (seen.contains(theme->id)) {
            problems.append(tr("%1 ends up extending itself").arg(nameOf(*theme)));
            break;
        }
        seen.append(theme->id);
        chain.prepend(theme);
        if (theme->id == kDefaultId)
            break;

        const QString base = theme->extends.isEmpty() ? kDefaultId : theme->extends;
        theme = find(base);
        if (!theme)
            problems.append(
                tr("%1 extends %2, which is not installed").arg(nameOf(*chain.constFirst()), base));
    }
    return chain;
}

QStringList ThemeLibrary::fontsOf(const QList<const ThemeFile *> &chain,
                                  QStringList &problems) const
{
    QStringList fonts;
    for (const ThemeFile *theme : chain) {
        const QString root = QDir::cleanPath(theme->directory());
        for (const QString &font : theme->fonts) {
            const QString path = QDir::cleanPath(QDir(root).absoluteFilePath(font));
            if (!path.startsWith(root + QLatin1Char('/'))) {
                problems.append(tr("The font %1 is outside the theme folder").arg(font));
                continue;
            }
            if (!fonts.contains(path))
                fonts.append(path);
        }
    }
    return fonts;
}

QHash<QString, int> ThemeLibrary::load(const QStringList &fonts, QStringList &problems) const
{
    QHash<QString, int> loaded;
    for (const QString &path : fonts) {
        int id = m_fonts.value(path, -1);
        if (id < 0)
            id = QFontDatabase::addApplicationFont(path);
        if (id < 0) {
            problems.append(tr("The font %1 could not be loaded").arg(QFileInfo(path).fileName()));
            continue;
        }
        loaded.insert(path, id);
    }
    return loaded;
}

ThemeScheme ThemeLibrary::schemeOf(const ThemeFile &leaf, const QList<const ThemeFile *> &chain,
                                   QStringList &problems) const
{
    ThemeScheme scheme;
    scheme.name = nameOf(leaf);
    scheme.location = builtIn(leaf.id) ? tr("Built into the app") : leaf.path;
    for (const ThemeFile *theme : chain) {
        if (theme->seed.isValid())
            scheme.seed = theme->seed;
        scheme.light.insert(theme->light);
        scheme.dark.insert(theme->dark);
        scheme.lightOverrides.insert(theme->lightOverrides);
        scheme.darkOverrides.insert(theme->darkOverrides);
    }

    const bool roles = !scheme.light.isEmpty() || !scheme.dark.isEmpty();
    const bool anchored = scheme.seed.isValid() || scheme.light.contains(kPrimaryRole)
        || scheme.dark.contains(kPrimaryRole);
    if (roles && !anchored)
        problems.append(tr("The colors need a seed or a primary color, so they are not offered"));
    return scheme;
}

QString ThemeLibrary::nameOf(const ThemeFile &theme) const
{
    const BuiltIn *entry = builtIn(theme.id);
    return entry ? tr(entry->name) : theme.name;
}

QString ThemeLibrary::captionOf(const ThemeFile &theme) const
{
    if (const BuiltIn *entry = builtIn(theme.id))
        return tr(entry->description);
    if (theme.author.isEmpty())
        return theme.description;
    if (theme.description.isEmpty())
        return tr("By %1").arg(theme.author);
    return tr("%1, by %2").arg(theme.description, theme.author);
}

}
