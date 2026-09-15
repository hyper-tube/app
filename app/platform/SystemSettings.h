#pragma once

#include <QObject>
#include <QQmlEngine>

namespace platform {

class SystemSettings : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool available READ available CONSTANT)
    Q_PROPERTY(Background background READ background CONSTANT)
    Q_PROPERTY(bool startupAvailable READ startupAvailable CONSTANT)
    Q_PROPERTY(bool closeToTray READ closeToTray WRITE setCloseToTray NOTIFY changed)
    Q_PROPERTY(bool launchAtSignIn READ launchAtSignIn WRITE setLaunchAtSignIn NOTIFY changed)

public:
    enum Background {
        Unavailable,
        Tray,
        Dock,
    };
    Q_ENUM(Background)

    explicit SystemSettings(QObject *parent);

    static SystemSettings &instance();
    static SystemSettings *create(QQmlEngine *, QJSEngine *);

    bool available() const;
    Background background() const;
    bool startupAvailable() const;
    bool closeToTray() const { return m_closeToTray; }
    bool launchAtSignIn() const { return m_launchAtSignIn; }

    void setCloseToTray(bool keep);
    void setLaunchAtSignIn(bool launch);

Q_SIGNALS:
    void changed();

private:
    bool m_closeToTray = true;
    bool m_launchAtSignIn = false;
};

}
