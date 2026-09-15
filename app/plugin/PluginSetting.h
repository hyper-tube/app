#pragma once

#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

namespace plugin {

struct PluginSetting
{
    enum Kind {
        Toggle,
        Select,
    };

    QString key;
    Kind kind = Toggle;
    QString label;
    QString caption;
    QVariant fallback;
    QVariantList options;

    QVariantMap describe() const;
};

QVariantMap selectOption(const QString &value, const QString &label);

}
