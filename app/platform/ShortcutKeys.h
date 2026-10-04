#pragma once

#include <QObject>
#include <QQmlEngine>

namespace platform {

class ShortcutKeys : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit ShortcutKeys(QObject *parent = nullptr);

    Q_INVOKABLE static int resolve(int key, quint32 nativeScanCode);
};

}
