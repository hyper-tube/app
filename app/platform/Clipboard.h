#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>

namespace platform {

class Clipboard : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit Clipboard(QObject *parent = nullptr);

    Q_INVOKABLE bool copy(const QString &text);
};

}
