#pragma once

#include "ThemeScheme.h"

#include <QObject>
#include <QQmlEngine>
#include <QVariantMap>

namespace theme {

class ActiveTheme : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QVariantMap values READ values NOTIFY valuesChanged)

public:
    explicit ActiveTheme(QObject *parent);

    static ActiveTheme &instance();
    static ActiveTheme *create(QQmlEngine *, QJSEngine *);

    const QVariantMap &values() const { return m_values; }
    const ThemeScheme &scheme() const { return m_scheme; }

    void apply(const QVariantMap &values, const ThemeScheme &scheme);

Q_SIGNALS:
    void valuesChanged();
    void schemeChanged();

private:
    QVariantMap m_values;
    ThemeScheme m_scheme;
};

}
