#include "ActiveTheme.h"

#include "Tokens.h"

#include <QCoreApplication>

namespace theme {

ActiveTheme::ActiveTheme(QObject *parent)
    : QObject(parent)
    , m_values(tokens::defaults())
{
}

ActiveTheme &ActiveTheme::instance()
{
    static auto *activeTheme = new ActiveTheme(QCoreApplication::instance());
    return *activeTheme;
}

ActiveTheme *ActiveTheme::create(QQmlEngine *, QJSEngine *)
{
    QQmlEngine::setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
    return &instance();
}

void ActiveTheme::apply(const QVariantMap &values, const ThemeScheme &scheme)
{
    if (m_values != values) {
        m_values = values;
        Q_EMIT valuesChanged();
    }
    if (m_scheme != scheme) {
        m_scheme = scheme;
        Q_EMIT schemeChanged();
    }
}

}
