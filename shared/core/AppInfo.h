#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QUrl>

namespace core {

class AppInfo : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString identifier READ identifier CONSTANT)
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(QString website READ website CONSTANT)
    Q_PROPERTY(QString repository READ repository CONSTANT)
    Q_PROPERTY(QString logFile READ logFile CONSTANT)
    Q_PROPERTY(QUrl logFolder READ logFolder CONSTANT)

public:
    explicit AppInfo(QObject *parent = nullptr);

    static void identify();

    static QString name();
    static QString identifier();
    static QString version();
    static QString website();
    static QString repository();
    static QString logFile();
    static QUrl logFolder();
};

}
