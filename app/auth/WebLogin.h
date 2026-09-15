#pragma once

#include "FirefoxAgent.h"

#include <QHash>
#include <QNetworkCookie>
#include <QObject>
#include <QQmlEngine>
#include <QQuickWebEngineProfile>
#include <QTimer>
#include <QUrl>

namespace auth {

class WebLogin : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QQuickWebEngineProfile *profile READ profile CONSTANT)
    Q_PROPERTY(QUrl signInUrl READ signInUrl CONSTANT)
    Q_PROPERTY(QUrl landingUrl READ landingUrl CONSTANT)
    Q_PROPERTY(bool harvesting READ harvesting NOTIFY harvestingChanged)

public:
    explicit WebLogin(QObject *parent);

    static WebLogin &instance();
    static WebLogin *create(QQmlEngine *, QJSEngine *);

    QQuickWebEngineProfile *profile() const { return m_profile; }
    QUrl signInUrl() const;
    QUrl landingUrl() const;
    bool harvesting() const { return m_poll.isActive(); }

    Q_INVOKABLE void follow(const QUrl &url);
    void forget();

Q_SIGNALS:
    void harvestingChanged();
    void harvested(bool credentialed);

private:
    void applyAgent();
    void harvest();
    void poll();
    QList<QNetworkCookie> youtubeCookies() const;

    FirefoxAgent *m_agent;
    QQuickWebEngineProfile *m_profile;
    QHash<QString, QNetworkCookie> m_seen;
    QTimer m_poll;
    int m_attempts = 0;
};

}
