#include "Session.h"

#include "auth/Credentials.h"
#include "core/Logging.h"
#include "net/CookieJar.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

namespace {

constexpr int kTrackingShutdownTimeoutMs = 1500;

const QString kBaseUrl = QStringLiteral("https://music.youtube.com/youtubei/v1/");
const QString kOrigin = QStringLiteral("https://music.youtube.com");
const QString kReferer = QStringLiteral("https://music.youtube.com/");
const QString kVisitorDataUrl = QStringLiteral("https://music.youtube.com/sw.js_data");

QString parseVisitorData(const QByteArray &body)
{
    const int start = body.indexOf('[');
    if (start < 0)
        return {};

    const QJsonDocument document = QJsonDocument::fromJson(body.mid(start));
    const QJsonArray candidates = document.array().at(0).toArray().at(2).toArray();
    for (const QJsonValue &value : candidates) {
        const QString text = value.toString();
        if (text.startsWith(QLatin1String("Cgt")) || text.startsWith(QLatin1String("Cgs")))
            return text;
    }
    return {};
}

QString errorFrom(const QJsonObject &json)
{
    const QJsonObject error = json.value(QStringLiteral("error")).toObject();
    if (error.isEmpty())
        return {};

    const QString message = error.value(QStringLiteral("message")).toString();
    return message.isEmpty() ? QStringLiteral("innertube error") : message;
}

}

namespace innertube {

Session::Session(QObject *parent)
    : QObject(parent)
    , m_http(net::HttpClient::instance())
    , m_authenticated(auth::credentials::present(m_http.cookies()))
{
    connect(&m_http.cookies(), &net::CookieJar::changed, this, [this] {
        const bool authenticated = auth::credentials::present(m_http.cookies());
        if (m_authenticated == authenticated)
            return;
        m_authenticated = authenticated;
        if (!authenticated)
            setIdentity({});
        Q_EMIT authenticatedChanged();
    });
}

Session &Session::instance()
{
    static auto *session = new Session(QCoreApplication::instance());
    return *session;
}

bool Session::authenticated() const
{
    return m_authenticated;
}

void Session::setIdentity(const QString &dataSyncId)
{
    if (m_dataSyncId == dataSyncId)
        return;
    m_dataSyncId = dataSyncId;
    const qsizetype separator = dataSyncId.indexOf(QLatin1String("||"));
    const QString head = separator < 0 ? dataSyncId : dataSyncId.left(separator);
    const QString tail = separator < 0 ? QString() : dataSyncId.mid(separator + 2);
    m_identity = head.isEmpty() ? tail : head;
    m_pageId = !head.isEmpty() && !tail.isEmpty() ? head : QString();
    Q_EMIT identityChanged();
}

void Session::call(const QString &endpoint, const Client &client, QJsonObject body,
                   const Handler &handler, bool background)
{
    if (!m_bootstrapped) {
        m_pending.append([this, endpoint, client, body, handler, background] {
            send(endpoint, client, body, handler, background);
        });
        bootstrap();
        return;
    }
    send(endpoint, client, std::move(body), handler, background);
}

void Session::ping(const Client &client, const QUrl &url)
{
    const net::Credentialed credentialed =
        signs(client) ? net::Credentialed::Yes : net::Credentialed::No;
    ++m_pendingPings;
    m_http.get(url, headersFor(client), credentialed, [this](const net::Response &response) {
        qCDebug(logInnerTube) << "tracking response" << response.status << response.ok();
        if (--m_pendingPings == 0)
            Q_EMIT trackingSettled();
    });
}

void Session::finishTrackingRequests() const
{
    if (m_pendingPings == 0)
        return;
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    connect(this, &Session::trackingSettled, &loop, &QEventLoop::quit);
    connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    timeout.start(kTrackingShutdownTimeoutMs);
    loop.exec(QEventLoop::ExcludeUserInputEvents);
}

void Session::bootstrap()
{
    if (m_bootstrapping)
        return;
    m_bootstrapping = true;

    m_http.get(QUrl(kVisitorDataUrl), {}, net::Credentialed::No,
               [this](const net::Response &response) {
        adoptVisitorData(response);
        m_bootstrapping = false;
        m_bootstrapped = !response.unreachable;
        releasePending();
    });
}

void Session::adoptVisitorData(const net::Response &response)
{
    if (response.unreachable) {
        qCDebug(logInnerTube) << "visitorData bootstrap deferred until the network is back";
        return;
    }
    if (!response.ok()) {
        qCWarning(logInnerTube) << "visitorData bootstrap failed" << response.error;
        return;
    }

    m_visitorData = parseVisitorData(response.body);
    if (m_visitorData.isEmpty())
        qCWarning(logInnerTube) << "visitorData missing from sw.js_data";
    else
        qCInfo(logInnerTube) << "session bootstrapped";
}

void Session::releasePending()
{
    const QList<std::function<void()>> pending = std::move(m_pending);
    m_pending.clear();
    for (const std::function<void()> &request : pending)
        request();
}

void Session::send(const QString &endpoint, const Client &client, QJsonObject body,
                   const Handler &handler, bool background)
{
    const bool authenticate = signs(client);
    body.insert(QStringLiteral("context"),
                m_context.build(client, m_visitorData, authenticate ? m_identity : QString()));

    QUrl url(kBaseUrl + endpoint);
    QUrlQuery query(url);
    query.addQueryItem(QStringLiteral("prettyPrint"), QStringLiteral("false"));
    url.setQuery(query);
    const QByteArray payload = QJsonDocument(body).toJson(QJsonDocument::Compact);

    m_http.post(url, headersFor(client), payload,
                authenticate ? net::Credentialed::Yes : net::Credentialed::No,
                [this, endpoint, authenticate, handler](const net::Response &response) {
        Reply reply;
        if (response.sessionRejected()) {
            if (authenticate) {
                qCWarning(logInnerTube) << endpoint << "rejected the signed-in session";
                Q_EMIT rejected();
            } else {
                qCWarning(logInnerTube)
                    << endpoint << "rejected the session, refetching visitorData";
                m_visitorData.clear();
                m_bootstrapped = false;
            }
        }

        if (!response.ok()) {
            reply.error = response.error;
            reply.unreachable = response.unreachable;
            handler(reply);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(response.body, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            reply.error = parseError.errorString();
            handler(reply);
            return;
        }

        reply.json = document.object();
        reply.error = errorFrom(reply.json);
        handler(reply);
    }, background);
}

bool Session::signs(const Client &client) const
{
    return client.loginSupported && m_authenticated;
}

net::Headers Session::headersFor(const Client &client) const
{
    net::Headers headers {
        {"content-type", "application/json"},
        {"accept", "application/json"},
        {"x-goog-api-format-version", "1"},
        {"x-youtube-client-name", client.id.toUtf8()},
        {"x-youtube-client-version", client.version.toUtf8()},
        {"x-origin", kOrigin.toUtf8()},
        {"referer", kReferer.toUtf8()},
        {"user-agent", client.userAgent.toUtf8()},
    };

    if (!m_visitorData.isEmpty())
        headers.append(net::Header {"x-goog-visitor-id", m_visitorData.toUtf8()});

    if (signs(client)) {
        headers.append(net::Header {"authorization",
                                    auth::credentials::authorization(m_http.cookies(), kOrigin)});
        headers.append(net::Header {"x-goog-authuser", "0"});
        if (!m_pageId.isEmpty())
            headers.append(net::Header {"x-goog-pageid", m_pageId.toUtf8()});
    }

    return headers;
}

}
