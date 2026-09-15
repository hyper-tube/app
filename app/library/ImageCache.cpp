#include "ImageCache.h"

#include "core/Logging.h"
#include "core/Paths.h"
#include "net/HttpClient.h"

#include <QBuffer>
#include <QCryptographicHash>
#include <QDir>
#include <QImageReader>
#include <QSaveFile>

#include <algorithm>
#include <cstddef>

namespace {

constexpr int kConcurrentRequests = 12;
constexpr int kMemoryLimitKiB = 48 * 1024;
constexpr qint64 kDiskLimit = 256 * 1024 * 1024;

QImage decode(const QByteArray &bytes, const QSize &target)
{
    QBuffer buffer;
    buffer.setData(bytes);
    buffer.open(QIODevice::ReadOnly);
    QImageReader reader(&buffer);
    const QSize size = reader.size();
    if (!size.isValid() || qint64(size.width()) * size.height() > 40000000)
        return {};
    const QSize scaled = size.scaled(target, Qt::KeepAspectRatioByExpanding);
    if (scaled.width() < size.width() || scaled.height() < size.height())
        reader.setScaledSize(scaled);
    reader.setAutoTransform(true);
    return reader.read();
}

}

namespace library {

ImageCache::ImageCache(QObject *parent)
    : QObject(parent)
    , m_memory(kMemoryLimitKiB)
    , m_directory(core::paths::artworkCacheDir())
{
    QMetaObject::invokeMethod(this, &ImageCache::pruneDisk, Qt::QueuedConnection);
}

QUrl ImageCache::sizedUrl(const QString &source, const QSize &size) const
{
    QUrl url(source);
    if (url.scheme().isEmpty() && source.startsWith(QLatin1String("//")))
        url.setScheme(QStringLiteral("https"));
    const QString host = url.host();
    if (host.endsWith(QLatin1String(".googleusercontent.com"))
        || host.endsWith(QLatin1String(".ggpht.com"))) {
        QString path = url.path();
        const qsizetype suffix = path.lastIndexOf(QLatin1Char('='));
        if (suffix >= 0)
            path.truncate(suffix);
        path += QStringLiteral("=w%1-h%2-l90-rj").arg(size.width()).arg(size.height());
        url.setPath(path);
    }
    if (host.endsWith(QLatin1String(".ytimg.com"))) {
        QStringList parts = url.path().split(QLatin1Char('/'));
        if (parts.size() == 4
            && (parts.at(1) == QLatin1String("vi") || parts.at(1) == QLatin1String("vi_webp"))) {
            const QString preset = qMax(size.width(), size.height()) <= 320
                ? QStringLiteral("mqdefault")
                : QStringLiteral("maxresdefault");
            const QString extension = parts.at(1) == QLatin1String("vi_webp")
                ? QStringLiteral(".webp")
                : QStringLiteral(".jpg");
            parts[3] = preset + extension;
            url.setPath(parts.join(QLatin1Char('/')));
            url.setQuery(QString());
        }
    }
    return url;
}

void ImageCache::load(const QString &source, const QSize &size, Handler handler, Needed needed)
{
    if (needed && !needed()) {
        handler({});
        return;
    }
    const QSize target(qBound(32, size.width(), 1024), qBound(32, size.height(), 1024));
    const QUrl url = sizedUrl(source, target);
    if (url.isLocalFile()) {
        QFile file(url.toLocalFile());
        handler(file.open(QIODevice::ReadOnly) ? decode(file.readAll(), target) : QImage());
        return;
    }
    if (url.scheme() != QLatin1String("https") || url.host().isEmpty()) {
        handler({});
        return;
    }
    const QByteArray identity = url.toEncoded() + QByteArray::number(target.width()) + 'x'
        + QByteArray::number(target.height());
    const QString key =
        QString::fromLatin1(QCryptographicHash::hash(identity, QCryptographicHash::Sha256).toHex());
    if (const QImage *cached = m_memory.object(key)) {
        handler(*cached);
        return;
    }
    if (m_pending.contains(key)) {
        m_pending[key].append({std::move(handler), std::move(needed)});
        return;
    }
    m_pending.insert(key, {{std::move(handler), std::move(needed)}});
    const QString path = m_directory + QLatin1Char('/') + key;
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        const QImage image = decode(file.readAll(), target);
        if (!image.isNull()) {
            finish(key, image);
            return;
        }
        file.close();
        file.remove();
    }
    m_requests.append({key, path, url, target});
    startNext();
}

void ImageCache::startNext()
{
    while (m_activeRequests < kConcurrentRequests && !m_requests.isEmpty()) {
        const Request request = m_requests.takeLast();
        if (!isNeeded(request.key)) {
            finish(request.key, {});
            continue;
        }
        ++m_activeRequests;
        net::HttpClient::instance().get(request.url, {}, net::Credentialed::No,
                                        [this, request](const net::Response &response) {
            --m_activeRequests;
            if (!isNeeded(request.key)) {
                finish(request.key, {});
                startNext();
                return;
            }
            const QImage image = response.ok() ? decode(response.body, request.size) : QImage();
            if (!image.isNull()) {
                QSaveFile file(request.path);
                if (file.open(QIODevice::WriteOnly)) {
                    file.write(response.body);
                    file.commit();
                }
                if (++m_writesSincePrune >= 128) {
                    m_writesSincePrune = 0;
                    pruneDisk();
                }
            } else {
                qCDebug(logArtwork) << "thumbnail unavailable" << response.status;
            }
            if (image.isNull()
                && request.url.fileName().startsWith(QLatin1String("maxresdefault"))) {
                Request fallback = request;
                QString path = fallback.url.path();
                path.replace(QStringLiteral("maxresdefault"), QStringLiteral("mqdefault"));
                fallback.url.setPath(path);
                m_requests.append(fallback);
            } else {
                finish(request.key, image);
            }
            startNext();
        });
    }
}

bool ImageCache::isNeeded(const QString &key) const
{
    const auto entry = m_pending.constFind(key);
    return entry != m_pending.cend()
        && std::any_of(entry->cbegin(), entry->cend(), [](const Listener &listener) {
        return !listener.needed || listener.needed();
    });
}

void ImageCache::finish(const QString &key, const QImage &image)
{
    if (!image.isNull())
        m_memory.insert(key, new QImage(image), qMax(1, int(image.sizeInBytes() / 1024)));
    const QList<Listener> listeners = m_pending.take(key);
    for (const Listener &listener : listeners)
        listener.handler(listener.needed && !listener.needed() ? QImage() : image);
}

void ImageCache::pruneDisk()
{
    const QFileInfoList files = QDir(m_directory).entryInfoList(QDir::Files, QDir::Time);
    qint64 total = 0;
    for (const QFileInfo &file : files) {
        total += file.size();
        if (total > kDiskLimit)
            QFile::remove(file.absoluteFilePath());
    }
}

}
