#pragma once

#include "innertube/Endpoints.h"

#include <QDeadlineTimer>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QTimer>

namespace auth {

class Account : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(Status status READ status NOTIFY changed)
    Q_PROPERTY(bool signedIn READ signedIn NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool renewing READ renewing NOTIFY changed)
    Q_PROPERTY(QString name READ name NOTIFY changed)
    Q_PROPERTY(QString handle READ handle NOTIFY changed)
    Q_PROPERTY(QString artId READ artId NOTIFY changed)
    Q_PROPERTY(QString channelId READ channelId NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)

public:
    enum Status {
        SignedOut,
        SigningIn,
        SignedIn,
        Expired,
    };
    Q_ENUM(Status)

    explicit Account(QObject *parent);

    static Account &instance();
    static Account *create(QQmlEngine *, QJSEngine *);

    Status status() const { return m_status; }
    bool signedIn() const { return m_status == SignedIn; }
    bool busy() const { return m_status == SigningIn; }
    bool renewing() const { return m_renewing; }
    const QString &name() const { return m_name; }
    const QString &handle() const { return m_handle; }
    const QString &artId() const { return m_artId; }
    const QString &channelId() const { return m_channelId; }
    const QString &error() const { return m_error; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void signOut();
    Q_INVOKABLE void requestSignIn();

Q_SIGNALS:
    void changed();
    void errorRaised();
    void signInRequested();
    void signInUnavailable();

private:
    void restore();
    void verify();
    void accept(const innertube::Reply &reply);
    void remember(const QString &identity) const;
    void renew();
    void endRenewal();
    void expire();
    void setStatus(Status status, const QString &error = {});

    innertube::Endpoints m_endpoints;
    QTimer m_renewalTimeout;
    QDeadlineTimer m_renewalCooldown;
    QString m_name;
    QString m_handle;
    QString m_artId;
    QString m_channelId;
    QString m_error;
    Status m_status = SignedOut;
    bool m_renewing = false;
    bool m_unverified = false;
};

}
