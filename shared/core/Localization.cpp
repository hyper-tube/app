#include "Localization.h"

#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QLocale>
#include <QSettings>
#include <QVariantMap>

namespace {

const QString kLanguageKey = QStringLiteral("localization/language");
const QString kSystem = QStringLiteral("system");
const QString kCatalogDir = QStringLiteral(":/i18n");

const QStringList kSupported {
    QStringLiteral("en"),
    QStringLiteral("ru"),
    QStringLiteral("uk"),
};

QString systemChoice()
{
    for (const QString &tag : QLocale::system().uiLanguages()) {
        const QString code = tag.left(tag.indexOf(QLatin1Char('-'))).toLower();
        if (kSupported.contains(code))
            return code;
    }
    return kSupported.constFirst();
}

QString storedLanguage()
{
    const QString stored = QSettings().value(kLanguageKey, kSystem).toString();
    return kSupported.contains(stored) ? stored : kSystem;
}

QString resolve(const QString &language)
{
    return kSupported.contains(language) ? language : systemChoice();
}

QString endonym(const QString &code)
{
    const QLocale locale(code);
    const QString unqualified = QLocale::languageToString(locale.language());
    const QString own = locale.nativeLanguageName();
    if (own.contains(unqualified))
        return unqualified;
    return locale.toUpper(own.left(1)) + own.mid(1);
}

QVariantMap option(const QString &value, const QString &label, const QString &caption = {})
{
    QVariantMap entry;
    entry.insert(QStringLiteral("value"), value);
    entry.insert(QStringLiteral("label"), label);
    if (!caption.isEmpty())
        entry.insert(QStringLiteral("caption"), caption);
    return entry;
}

}

namespace core {

Localization::Localization(QObject *parent)
    : QObject(parent)
    , m_language(storedLanguage())
{
    adopt(resolve(m_language));
    if (QCoreApplication *application = QCoreApplication::instance())
        application->installEventFilter(this);
}

Localization &Localization::instance()
{
    static auto *localization = new Localization(QCoreApplication::instance());
    return *localization;
}

Localization *Localization::create(QQmlEngine *, QJSEngine *)
{
    QQmlEngine::setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
    return &instance();
}

QVariantList Localization::options() const
{
    QVariantList entries;
    entries.append(option(kSystem, tr("System"), endonym(systemChoice())));
    for (const QString &code : kSupported)
        entries.append(option(code, endonym(code)));
    return entries;
}

void Localization::setLanguage(const QString &language)
{
    const QString chosen = kSupported.contains(language) ? language : kSystem;
    if (m_language == chosen)
        return;

    m_language = chosen;
    QSettings().setValue(kLanguageKey, m_language);
    adopt(resolve(m_language));
    Q_EMIT changed();
}

bool Localization::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::LocaleChange && m_language == kSystem) {
        adopt(systemChoice());
        Q_EMIT changed();
    }
    return QObject::eventFilter(watched, event);
}

void Localization::adopt(const QString &resolved)
{
    if (m_resolved == resolved)
        return;

    m_resolved = resolved;
    for (QTranslator *catalog : std::as_const(m_catalogs)) {
        QCoreApplication::removeTranslator(catalog);
        delete catalog;
    }
    m_catalogs.clear();

    const QStringList files =
        QDir(kCatalogDir)
            .entryList({QStringLiteral("*_") + m_resolved + QStringLiteral(".qm")}, QDir::Files);
    for (const QString &file : files) {
        auto *catalog = new QTranslator(this);
        if (!catalog->load(kCatalogDir + QLatin1Char('/') + file)) {
            delete catalog;
            continue;
        }
        QCoreApplication::installTranslator(catalog);
        m_catalogs.append(catalog);
    }
    QLocale::setDefault(QLocale(m_resolved));
    Q_EMIT resolvedChanged();
}

}
