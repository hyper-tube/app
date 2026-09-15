#include "PluginSetting.h"

namespace {

QString kindName(plugin::PluginSetting::Kind kind)
{
    switch (kind) {
    case plugin::PluginSetting::Select: return QStringLiteral("select");
    case plugin::PluginSetting::Toggle: break;
    }
    return QStringLiteral("toggle");
}

}

namespace plugin {

QVariantMap PluginSetting::describe() const
{
    return {
        {QStringLiteral("key"), key},         {QStringLiteral("kind"), kindName(kind)},
        {QStringLiteral("label"), label},     {QStringLiteral("caption"), caption},
        {QStringLiteral("options"), options},
    };
}

QVariantMap selectOption(const QString &value, const QString &label)
{
    return {{QStringLiteral("value"), value}, {QStringLiteral("label"), label}};
}

}
