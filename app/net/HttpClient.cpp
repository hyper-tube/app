#include "HttpClient.h"

#include "Connectivity.h"
#include "CookieJar.h"
#include "DownloadTransfer.h"
#include "core/AppInfo.h"
#include "core/Logging.h"
#include "diagnostics/Diagnostics.h"

#include <QCoreApplication>
#include <QMetaEnum>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace {

constexpr int kMinimumIntervalMs = 100;
constexpr int kTransferTimeoutMs = 20000;

constexpr QLatin1StringView kInnerTubePath("/youtubei/v1/");

const char *surfaceOf(const QString &host, const QString &path)
{
    if (host == QLatin1StringView("music.youtube.com")) {
        if (path.startsWith(QLatin1StringView("/api/stats")))
            return "playback_tracking";
        return "music_web";
    }
    if (host.endsWith(QLatin1StringView(".googlevideo.com")))
        return "media";
    if (host == QLatin1StringView("upload.youtube.com"))
        return "upload";
    if (host.endsWith(QLatin1StringView("youtube.com")))
        return path.startsWith(QLatin1StringView("/api/stats")) ? "playback_tracking"
                                                                : "youtube_web";
    if (host.endsWith(QLatin1StringView(".ytimg.com"))
        || host.endsWith(QLatin1StringView(".ggpht.com"))
        || host.endsWith(QLatin1StringView(".googleusercontent.com")))
        return "artwork";
    if (host.endsWith(QLatin1StringView(".mozilla.org")))
        return "browser_versions";
    if (host == QUrl(core::AppInfo::website()).host())
        return "website";
    return "other";
}

diagnostics::Value endpointOf(const QUrl &url)
{
    const QString path = url.path();
    if (url.host() == QLatin1StringView("music.youtube.com") && path.startsWith(kInnerTubePath))
        return diagnostics::Value::symbol(QStringView(path).sliced(kInnerTubePath.size()));
    return diagnostics::Value(surfaceOf(url.host(), path));
}

const char *reasonOf(QNetworkReply::NetworkError error)
{
    const char *name = QMetaEnum::fromType<QNetworkReply::NetworkError>().valueToKey(error);
    return name ? name : "unknown";
}

}

namespace net {

QByteArray Response::header(const QByteArray &name) const
{
    for (const Header &entry : headers) {
        if (entry.name.compare(name, Qt::CaseInsensitive) == 0)
            return entry.value;
    }
    return {};
}

HttpClient::HttpClient(QObject *parent)
    : QObject(parent)
    , m_limiter(kMinimumIntervalMs, this)
    , m_cookies(new CookieJar(this))
{
    m_manager.setCookieJar(m_cookies);
}

HttpClient &HttpClient::instance()
{
    static auto *client = new HttpClient(QCoreApplication::instance());
    return *client;
}

DownloadTransfer *HttpClient::download(const QUrl &url, const Headers &headers, const QString &path,
                                       qint64 offset, qint64 maximumBytes)
{
    auto *transfer = new DownloadTransfer(path, offset, maximumBytes, this);
    const QPointer<DownloadTransfer> guard(transfer);
    m_limiter.schedule([this, url, headers, guard] {
        if (!guard || guard->cancelled())
            return;
        guard->start(m_manager.get(buildRequest(url, headers, Credentialed::No)));
    }, true);
    return transfer;
}

void HttpClient::get(const QUrl &url, const Headers &headers, Credentialed credentialed,
                     Handler handler, bool background)
{
    if (credentialed == Credentialed::No
        && (url.host().endsWith(QLatin1String(".ytimg.com"))
            || url.host().endsWith(QLatin1String(".googleusercontent.com"))
            || url.host().endsWith(QLatin1String(".ggpht.com")))) {
        deliver(m_manager.get(buildRequest(url, headers, credentialed)), handler, false);
        return;
    }
    m_limiter.schedule([this, url, headers, credentialed, handler = std::move(handler)] {
        qCDebug(logNet) << "GET" << url.toString(QUrl::RemoveQuery);
        deliver(m_manager.get(buildRequest(url, headers, credentialed)), handler);
    }, background);
}

void HttpClient::post(const QUrl &url, const Headers &headers, const QByteArray &body,
                      Credentialed credentialed, Handler handler, bool background)
{
    m_limiter.schedule([this, url, headers, body, credentialed, handler = std::move(handler)] {
        qCDebug(logNet) << "POST" << url.toString(QUrl::RemoveQuery) << body.size() << "bytes";
        deliver(m_manager.post(buildRequest(url, headers, credentialed), body), handler);
    }, background);
}

void HttpClient::upload(const QUrl &url, const Headers &headers, QIODevice *body,
                        std::function<void(qint64, qint64)> progress, Handler handler)
{
    m_limiter.schedule(
        [this, url, headers, body, progress = std::move(progress), handler = std::move(handler)] {
        qCDebug(logNet) << "UPLOAD" << url.host() << body->size() << "bytes";
        QNetworkRequest request = buildRequest(url, headers, Credentialed::Yes);
        request.setAttribute(QNetworkRequest::DoNotBufferUploadDataAttribute, true);
        request.setHeader(QNetworkRequest::ContentLengthHeader, body->size());
        QNetworkReply *reply = m_manager.post(request, body);
        if (progress)
            connect(reply, &QNetworkReply::uploadProgress, this, progress);
        deliver(reply, handler);
    });
}

QNetworkRequest HttpClient::buildRequest(const QUrl &url, const Headers &headers,
                                         Credentialed credentialed) const
{
    QNetworkRequest request(url);
    request.setTransferTimeout(kTransferTimeoutMs);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    if (credentialed == Credentialed::No)
        request.setAttribute(QNetworkRequest::CookieLoadControlAttribute, QNetworkRequest::Manual);
    for (const Header &header : headers)
        request.setRawHeader(header.name, header.value);
    return request;
}

void HttpClient::deliver(QNetworkReply *reply, const Handler &handler, bool reportErrors)
{
    connect(reply, &QNetworkReply::finished, this, [reply, handler, reportErrors] {
        Response response;
        response.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        response.body = reply->readAll();
        for (const QByteArray &name : reply->rawHeaderList())
            response.headers.append(Header {name, reply->rawHeader(name)});
        if (reply->error() != QNetworkReply::NoError)
            response.error = reply->errorString();
        response.unreachable = response.status == 0 && Connectivity::unreachable(reply->error());
        Connectivity::instance().observe(reply->error(), response.status);
        reply->deleteLater();

        if (response.unreachable)
            qCDebug(logNet) << "request could not reach the network" << int(reply->error());
        else if (reportErrors && !response.ok())
            qCWarning(logNet) << "request failed" << response.status << int(reply->error());

        if (reportErrors && !response.ok())
            diagnostics::breadcrumb("network.request_failed",
                                    {{"endpoint", endpointOf(reply->request().url())},
                                     {"status", response.status},
                                     {"reason", reasonOf(reply->error())},
                                     {"unreachable", response.unreachable}},
                                    diagnostics::Level::Warning);

        handler(response);
    });
}

}
