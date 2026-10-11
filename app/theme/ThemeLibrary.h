#pragma once

#include "ThemeFile.h"
#include "ThemeScheme.h"

#include <QFileSystemWatcher>
#include <QHash>
#include <QList>
#include <QObject>
#include <QQmlEngine>
#include <QStringList>
#include <QTimer>
#include <QVariantList>

namespace theme {

class ThemeLibrary : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QVariantList options READ options NOTIFY listChanged)
    Q_PROPERTY(QString current READ current NOTIFY currentChanged)
    Q_PROPERTY(QString caption READ caption NOTIFY currentChanged)
    Q_PROPERTY(QString name READ name NOTIFY currentChanged)
    Q_PROPERTY(QStringList problems READ problems NOTIFY problemsChanged)

public:
    explicit ThemeLibrary(QObject *parent);

    static ThemeLibrary &instance();
    static ThemeLibrary *create(QQmlEngine *, QJSEngine *);

    QVariantList options() const;
    const QString &current() const { return m_current; }
    QString caption() const;
    QString name() const;
    const QStringList &problems() const { return m_problems; }
    static QString folder();

    void restore();

    Q_INVOKABLE void select(const QString &id);
    Q_INVOKABLE void openFolder() const;

Q_SIGNALS:
    void listChanged();
    void currentChanged();
    void problemsChanged();

private:
    void reload();
    void scan();
    void watch();
    void apply();
    void writeSchema() const;
    void adoptProblems(const QStringList &problems);

    const ThemeFile *find(const QString &id) const;
    QList<const ThemeFile *> chainOf(const ThemeFile &leaf, QStringList &problems) const;
    QStringList fontsOf(const QList<const ThemeFile *> &chain, QStringList &problems) const;
    QHash<QString, int> load(const QStringList &fonts, QStringList &problems) const;
    ThemeScheme schemeOf(const ThemeFile &leaf, const QList<const ThemeFile *> &chain,
                         QStringList &problems) const;
    QString nameOf(const ThemeFile &theme) const;
    QString captionOf(const ThemeFile &theme) const;

    QList<ThemeFile> m_themes;
    QString m_chosen;
    QString m_current;
    QStringList m_problems;
    QHash<QString, int> m_fonts;
    QFileSystemWatcher m_watcher;
    QTimer m_settle;
};

}
