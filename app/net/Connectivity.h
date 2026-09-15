#pragma once

#include <QNetworkAccessManager>
#include <QNetworkInformation>
#include <QNetworkReply>
#include <QObject>
#include <QPointer>
#include <QQmlEngine>
#include <QTimer>

namespace net {

class Connectivity : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool online READ online NOTIFY onlineChanged)
    Q_PROPERTY(bool checking READ checking NOTIFY checkingChanged)

public:
    explicit Connectivity(QObject *parent);

    static Connectivity &instance();
    static Connectivity *create(QQmlEngine *, QJSEngine *);
    static bool unreachable(QNetworkReply::NetworkError error);

    bool online() const { return m_online; }
    bool checking() const { return !m_probe.isNull(); }
    bool settled() const { return m_online && !checking(); }

    void observe(QNetworkReply::NetworkError error, int status);
    Q_INVOKABLE void check();

Q_SIGNALS:
    void onlineChanged();
    void checkingChanged();

private:
    void follow(QNetworkInformation::Reachability reachability);
    void settle(bool reachable);
    void setOnline(bool online);

    QNetworkAccessManager m_manager;
    QPointer<QNetworkReply> m_probe;
    QTimer m_retry;
    int m_retryMs = 0;
    bool m_online = true;
    bool m_reachedWhileChecking = false;
};

}
