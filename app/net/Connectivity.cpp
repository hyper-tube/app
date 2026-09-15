#include "Connectivity.h"

#include "core/Logging.h"

#include <QCoreApplication>
#include <QNetworkRequest>
#include <QUrl>

namespace {

constexpr int kProbeTimeoutMs = 6000;
constexpr int kFirstRetryMs = 3000;
constexpr int kMaximumRetryMs = 30000;

const QString kProbeUrl = QStringLiteral("https://music.youtube.com/generate_204");

}

namespace net {

Connectivity::Connectivity(QObject *parent)
    : QObject(parent)
{
    m_retry.setSingleShot(true);
    connect(&m_retry, &QTimer::timeout, this, &Connectivity::check);

    if (!QNetworkInformation::loadBackendByFeatures(QNetworkInformation::Feature::Reachability))
        return;
    const QNetworkInformation *information = QNetworkInformation::instance();
    connect(information, &QNetworkInformation::reachabilityChanged, this, &Connectivity::follow);
    if (information->reachability() == QNetworkInformation::Reachability::Disconnected)
        check();
}

Connectivity &Connectivity::instance()
{
    static auto *connectivity = new Connectivity(QCoreApplication::instance());
    return *connectivity;
}

Connectivity *Connectivity::create(QQmlEngine *, QJSEngine *)
{
    QQmlEngine::setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
    return &instance();
}

bool Connectivity::unreachable(QNetworkReply::NetworkError error)
{
    switch (error) {
    case QNetworkReply::ConnectionRefusedError:
    case QNetworkReply::RemoteHostClosedError:
    case QNetworkReply::HostNotFoundError:
    case QNetworkReply::TimeoutError:
    case QNetworkReply::OperationCanceledError:
    case QNetworkReply::TemporaryNetworkFailureError:
    case QNetworkReply::NetworkSessionFailedError:
    case QNetworkReply::UnknownNetworkError:
    case QNetworkReply::ProxyConnectionRefusedError:
    case QNetworkReply::ProxyConnectionClosedError:
    case QNetworkReply::ProxyNotFoundError:
    case QNetworkReply::ProxyTimeoutError: return true;
    default: return false;
    }
}

void Connectivity::observe(QNetworkReply::NetworkError error, int status)
{
    if (status > 0 || error == QNetworkReply::NoError) {
        if (checking())
            m_reachedWhileChecking = true;
        else
            settle(true);
        return;
    }
    if (unreachable(error))
        check();
}

void Connectivity::check()
{
    m_retry.stop();
    if (checking())
        return;

    QNetworkRequest request {QUrl(kProbeUrl)};
    request.setTransferTimeout(kProbeTimeoutMs);
    request.setAttribute(QNetworkRequest::CookieLoadControlAttribute, QNetworkRequest::Manual);
    request.setAttribute(QNetworkRequest::CookieSaveControlAttribute, QNetworkRequest::Manual);
    QNetworkReply *reply = m_manager.get(request);
    m_probe = reply;
    m_reachedWhileChecking = false;
    Q_EMIT checkingChanged();

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        reply->deleteLater();
        m_probe.clear();
        settle(status > 0 || m_reachedWhileChecking);
        Q_EMIT checkingChanged();
    });
}

void Connectivity::follow(QNetworkInformation::Reachability reachability)
{
    qCDebug(logNet) << "system reachability is now" << reachability;
    m_retryMs = 0;
    check();
}

void Connectivity::settle(bool reachable)
{
    if (reachable) {
        m_retry.stop();
        m_retryMs = 0;
    } else {
        m_retryMs = m_retryMs == 0 ? kFirstRetryMs : qMin(m_retryMs * 2, kMaximumRetryMs);
        m_retry.start(m_retryMs);
    }
    setOnline(reachable);
}

void Connectivity::setOnline(bool online)
{
    if (m_online == online)
        return;
    m_online = online;
    qCInfo(logNet) << (online ? "network reachable again" : "network unreachable, working offline");
    Q_EMIT onlineChanged();
}

}
