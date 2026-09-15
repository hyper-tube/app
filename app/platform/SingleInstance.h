#pragma once

#include <QLocalServer>
#include <QObject>

namespace platform {

class SingleInstance : public QObject
{
    Q_OBJECT

public:
    explicit SingleInstance(QObject *parent = nullptr);

    bool claim();

Q_SIGNALS:
    void raiseRequested();

private:
    QLocalServer m_server;
};

}
