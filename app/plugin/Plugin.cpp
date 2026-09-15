#include "Plugin.h"

#include "PluginStore.h"
#include "core/Localization.h"

namespace {

const QString kModuleRoot = QStringLiteral("qrc:/qt/qml/HtMusic/App");

}

namespace plugin {

Plugin::Plugin(QObject *parent)
    : QObject(parent)
{
    connect(&core::Localization::instance(), &core::Localization::resolvedChanged, this,
            &Plugin::retranslated);
    connect(this, &Plugin::retranslated, this, &Plugin::restate);
}

QString Plugin::qmlSource(const QString &id, const QString &component)
{
    return kModuleRoot + QStringLiteral("/plugin/%1/qml/%2.qml").arg(id, component);
}

QString Plugin::assetSource(const QString &id, const QString &file)
{
    return kModuleRoot + QStringLiteral("/plugin/%1/assets/%2").arg(id, file);
}

QVariant Plugin::coerce(const QVariant &value, PluginSetting::Kind kind)
{
    switch (kind) {
    case PluginSetting::Toggle: return value.toBool();
    case PluginSetting::Select: break;
    }
    return value.toString();
}

QVariantList Plugin::settings() const
{
    QVariantList described;
    for (const PluginSetting &entry : schema())
        described.append(entry.describe());
    return described;
}

QVariant Plugin::value(const QString &key) const
{
    return m_values.value(key);
}

void Plugin::setValue(const QString &key, const QVariant &value)
{
    QVariant adopted = value;
    for (const PluginSetting &entry : schema()) {
        if (entry.key == key)
            adopted = coerce(value, entry.kind);
    }
    if (m_values.value(key) == adopted)
        return;
    m_values.insert(key, adopted);
    PluginStore(info().id).setValue(key, adopted);
    Q_EMIT valuesChanged();
    valueChanged(key);
}

void Plugin::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;
    m_enabled = enabled;
    PluginStore(info().id).setEnabled(m_enabled);
    m_running = m_enabled;
    if (m_running)
        start();
    else
        stop();
    Q_EMIT enabledChanged();
}

void Plugin::restore()
{
    const PluginStore store(info().id);
    for (const PluginSetting &entry : schema())
        m_values.insert(entry.key, coerce(store.value(entry.key, entry.fallback), entry.kind));
    Q_EMIT valuesChanged();
    setEnabled(store.enabled());
}

void Plugin::shutDown()
{
    if (!m_running)
        return;
    m_running = false;
    stop();
}

void Plugin::setState(Health health, const QString &status)
{
    if (m_health == health && m_status == status)
        return;
    m_health = health;
    m_status = status;
    Q_EMIT stateChanged();
}

void Plugin::valueChanged(const QString &key)
{
    Q_UNUSED(key)
}

void Plugin::restate() { }

}
