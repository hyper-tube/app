#pragma once

#include "PluginInfo.h"
#include "PluginSetting.h"

#include <QList>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

namespace plugin {

class Plugin : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Plugins are owned by PluginRegistry")

    Q_PROPERTY(plugin::PluginInfo info READ info NOTIFY retranslated)
    Q_PROPERTY(QVariantList settings READ settings NOTIFY retranslated)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QVariantMap values READ values NOTIFY valuesChanged)
    Q_PROPERTY(Health health READ health NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)

public:
    enum Health {
        Ok,
        Busy,
        Warning,
        Error,
    };
    Q_ENUM(Health)

    explicit Plugin(QObject *parent);

    virtual PluginInfo info() const = 0;

    bool enabled() const { return m_enabled; }
    const QVariantMap &values() const { return m_values; }
    Health health() const { return m_health; }
    const QString &status() const { return m_status; }
    QVariantList settings() const;

    void setEnabled(bool enabled);
    void restore();
    void shutDown();

    Q_INVOKABLE QVariant value(const QString &key) const;
    Q_INVOKABLE void setValue(const QString &key, const QVariant &value);

    static QString qmlSource(const QString &id, const QString &component);
    static QString assetSource(const QString &id, const QString &file);

Q_SIGNALS:
    void enabledChanged();
    void valuesChanged();
    void stateChanged();
    void retranslated();

protected:
    virtual QList<PluginSetting> schema() const { return {}; }
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void valueChanged(const QString &key);
    virtual void restate();

    void setState(Health health, const QString &status);

private:
    static QVariant coerce(const QVariant &value, PluginSetting::Kind kind);

    QVariantMap m_values;
    QString m_status;
    Health m_health = Ok;
    bool m_enabled = false;
    bool m_running = false;
};

}
