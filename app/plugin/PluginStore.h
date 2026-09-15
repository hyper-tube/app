#pragma once

#include <QString>
#include <QVariant>

namespace plugin {

class PluginStore
{
public:
    explicit PluginStore(QString id);

    bool enabled() const;
    void setEnabled(bool enabled);

    QVariant value(const QString &key, const QVariant &fallback) const;
    void setValue(const QString &key, const QVariant &value);

private:
    QString path(const QString &key) const;

    QString m_id;
};

}
