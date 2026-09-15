#include "PluginStore.h"

#include <QSettings>

#include <utility>

namespace {

const QString kEnabledKey = QStringLiteral("enabled");

}

namespace plugin {

PluginStore::PluginStore(QString id)
    : m_id(std::move(id))
{
}

QString PluginStore::path(const QString &key) const
{
    return QStringLiteral("plugins/") + m_id + QLatin1Char('/') + key;
}

bool PluginStore::enabled() const
{
    return QSettings().value(path(kEnabledKey), false).toBool();
}

void PluginStore::setEnabled(bool enabled)
{
    QSettings().setValue(path(kEnabledKey), enabled);
}

QVariant PluginStore::value(const QString &key, const QVariant &fallback) const
{
    return QSettings().value(path(key), fallback);
}

void PluginStore::setValue(const QString &key, const QVariant &value)
{
    QSettings().setValue(path(key), value);
}

}
