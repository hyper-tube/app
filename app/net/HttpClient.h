#pragma once

#include "RateLimiter.h"

#include <QByteArray>
#include <QList>
#include <QNetworkAccessManager>
#include <QObject>
#include <QUrl>

#include <functional>

class QIODevice;
class QNetworkReply;
class QNetworkRequest;

namespace net {

class CookieJar;
class DownloadTransfer;

struct Header
{
    QByteArray name;
    QByteArray value;
};

using Headers = QList<Header>;

struct Response
{
    int status = 0;
    QByteArray body;
    QString error;
    Headers headers;
    bool unreachable = false;

    QByteArray header(const QByteArray &name) const;
    bool ok() const { return error.isEmpty(); }
    bool sessionRejected() const { return status == 401 || status == 403; }
};

enum class Credentialed {
    No,
    Yes,
};

class HttpClient : public QObject
{
    Q_OBJECT

public:
    using Handler = std::function<void(const Response &)>;

    explicit HttpClient(QObject *parent = nullptr);

    static HttpClient &instance();

    CookieJar &cookies() const { return *m_cookies; }

    DownloadTransfer *download(const QUrl &url, const Headers &headers, const QString &path,
                               qint64 offset, qint64 maximumBytes);
    void get(const QUrl &url, const Headers &headers, Credentialed credentialed, Handler handler,
             bool background = false);
    void post(const QUrl &url, const Headers &headers, const QByteArray &body,
              Credentialed credentialed, Handler handler, bool background = false);
    void upload(const QUrl &url, const Headers &headers, QIODevice *body,
                std::function<void(qint64, qint64)> progress, Handler handler);

private:
    QNetworkRequest buildRequest(const QUrl &url, const Headers &headers,
                                 Credentialed credentialed) const;
    void deliver(QNetworkReply *reply, const Handler &handler, bool reportErrors = true);

    QNetworkAccessManager m_manager;
    RateLimiter m_limiter;
    CookieJar *m_cookies;
};

}
