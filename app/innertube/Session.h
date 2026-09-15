#pragma once

#include "ClientRegistry.h"
#include "ContextBuilder.h"
#include "net/HttpClient.h"

#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QString>

#include <functional>

namespace innertube {

struct Reply
{
    QJsonObject json;
    QString error;
    bool unreachable = false;

    bool ok() const { return error.isEmpty(); }
};

class Session : public QObject
{
    Q_OBJECT

public:
    using Handler = std::function<void(const Reply &)>;

    explicit Session(QObject *parent = nullptr);

    static Session &instance();

    const ClientRegistry &clients() const { return m_clients; }
    bool authenticated() const;
    const QString &identity() const { return m_identity; }

    void setIdentity(const QString &dataSyncId);
    void call(const QString &endpoint, const Client &client, QJsonObject body,
              const Handler &handler, bool background = false);
    void ping(const Client &client, const QUrl &url);
    void finishTrackingRequests() const;

Q_SIGNALS:
    void authenticatedChanged();
    void identityChanged();
    void trackingSettled();
    void rejected();

private:
    void bootstrap();
    void adoptVisitorData(const net::Response &response);
    void releasePending();
    void send(const QString &endpoint, const Client &client, QJsonObject body,
              const Handler &handler, bool background);
    bool signs(const Client &client) const;
    net::Headers headersFor(const Client &client) const;

    net::HttpClient &m_http;
    ClientRegistry m_clients;
    ContextBuilder m_context;
    QString m_visitorData;
    QString m_identity;
    QString m_dataSyncId;
    QString m_pageId;
    QList<std::function<void()>> m_pending;
    int m_pendingPings = 0;
    bool m_authenticated = false;
    bool m_bootstrapping = false;
    bool m_bootstrapped = false;
};

}
