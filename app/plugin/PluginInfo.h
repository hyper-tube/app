#pragma once

#include <QMetaType>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace plugin {

class PluginInfo
{
    Q_GADGET
    QML_VALUE_TYPE(pluginInfo)

    Q_PROPERTY(QString id MEMBER id)
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString description MEMBER description)
    Q_PROPERTY(QString about MEMBER about)
    Q_PROPERTY(QString icon MEMBER icon)
    Q_PROPERTY(QString iconSource MEMBER iconSource)
    Q_PROPERTY(QString cardSource MEMBER cardSource)
    Q_PROPERTY(QString settingsSource MEMBER settingsSource)

public:
    QString id;
    QString name;
    QString description;
    QString about;
    QString icon;
    QString iconSource;
    QString cardSource;
    QString settingsSource;

    bool operator==(const PluginInfo &other) const { return id == other.id; }
};

}

Q_DECLARE_METATYPE(plugin::PluginInfo)
