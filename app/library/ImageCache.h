#pragma once

#include <QCache>
#include <QHash>
#include <QImage>
#include <QObject>
#include <QSize>
#include <QUrl>

#include <functional>
#include <optional>

namespace net {
struct Response;
}

namespace library {

class ImageCache : public QObject
{
    Q_OBJECT

public:
    using Handler = std::function<void(const QImage &)>;
    using Needed = std::function<bool()>;

    explicit ImageCache(QObject *parent = nullptr);

    void load(const QString &source, const QSize &size, Handler handler, Needed needed = {});

private:
    struct Listener
    {
        Handler handler;
        Needed needed;
    };

    struct Entry
    {
        QUrl url;
        QByteArray body;
        QByteArray etag;
        QByteArray lastModified;
        qint64 expiresAt = 0;

        static Entry read(const QString &path, const QUrl &url);
        QByteArray serialized() const;
    };

    struct Decoded
    {
        QImage image;
        qint64 expiresAt = 0;
    };

    struct Request
    {
        QString key;
        QString path;
        QSize size;
        Entry entry;
    };

    void startNext();
    void complete(const Request &request, const net::Response &response, qint64 requestedAt);
    bool isNeeded(const QString &key) const;
    void finish(const QString &key, const QImage &image,
                std::optional<qint64> expiresAt = std::nullopt);
    void store(const QString &path, const Entry &entry);
    void pruneDisk();
    QUrl sizedUrl(const QString &source, const QSize &size) const;

    QCache<QString, Decoded> m_memory;
    QHash<QString, QList<Listener>> m_pending;
    QList<Request> m_requests;
    QString m_directory;
    int m_activeRequests = 0;
    int m_writesSincePrune = 0;
};

}
