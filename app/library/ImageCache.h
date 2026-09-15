#pragma once

#include <QCache>
#include <QHash>
#include <QImage>
#include <QObject>
#include <QSize>
#include <QUrl>

#include <functional>

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

    struct Request
    {
        QString key;
        QString path;
        QUrl url;
        QSize size;
    };

    void startNext();
    bool isNeeded(const QString &key) const;
    void finish(const QString &key, const QImage &image);
    void pruneDisk();
    QUrl sizedUrl(const QString &source, const QSize &size) const;

    QCache<QString, QImage> m_memory;
    QHash<QString, QList<Listener>> m_pending;
    QList<Request> m_requests;
    QString m_directory;
    int m_activeRequests = 0;
    int m_writesSincePrune = 0;
};

}
