#include "ImageCache.h"

#include "core/Logging.h"
#include "core/Paths.h"
#include "net/Connectivity.h"
#include "net/HttpClient.h"

#include <QBuffer>
#include <QCryptographicHash>
#include <QDataStream>
#include <QDateTime>
#include <QDir>
#include <QImageReader>
#include <QLocale>
#include <QSaveFile>
#include <QTimeZone>

#include <algorithm>
#include <cstddef>

namespace {

constexpr int kConcurrentRequests = 12;
constexpr int kMemoryLimitKiB = 48 * 1024;
constexpr qint64 kDiskLimit = 256 * 1024 * 1024;
constexpr quint32 kEntryFormat = 0x68746d31;
constexpr qint64 kDeltaSecondsLimit = 2147483648;

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

qint64 now()
{
    return QDateTime::currentMSecsSinceEpoch();
}

std::optional<qint64> parseHttpDate(const QByteArray &value)
{
    const QString text = QString::fromLatin1(value.trimmed());
    QDateTime date =
        QLocale::c().toDateTime(text, QStringLiteral("ddd, dd MMM yyyy HH:mm:ss 'GMT'"));

    if (date.isValid())
        date.setTimeZone(QTimeZone::UTC);
    else
        date = QDateTime::fromString(text, Qt::RFC2822Date);

    if (!date.isValid())
        return std::nullopt;

    return date.toMSecsSinceEpoch();
}

std::optional<qint64> millisecondsOf(const QByteArray &deltaSeconds)
{
    bool ok = false;
    const qint64 seconds = deltaSeconds.trimmed().toLongLong(&ok);
    if (!ok || seconds < 0)
        return std::nullopt;

    return std::min(seconds, kDeltaSecondsLimit) * 1000;
}

std::optional<qint64> expiryOf(const net::Response &response, qint64 requestedAt, qint64 receivedAt)
{
    bool noCache = false;
    std::optional<qint64> maxAge;

    for (const QByteArray &part : response.header("cache-control").split(',')) {
        const QByteArray directive = part.trimmed().toLower();
        if (directive == "no-store")
            return std::nullopt;
        if (directive == "no-cache" || directive.startsWith("no-cache="))
            noCache = true;
        else if (directive.startsWith("max-age=") && !maxAge)
            maxAge = millisecondsOf(directive.sliced(8).replace('"', "")).value_or(0);
    }

    const qint64 date = parseHttpDate(response.header("date")).value_or(receivedAt);
    qint64 lifetime = parseHttpDate(response.header("expires")).value_or(date) - date;

    if (noCache)
        lifetime = 0;
    else if (maxAge)
        lifetime = *maxAge;

    const qint64 apparentAge = std::max<qint64>(0, receivedAt - date);
    const qint64 correctedAge =
        millisecondsOf(response.header("age")).value_or(0) + receivedAt - requestedAt;

    return receivedAt + lifetime - std::max(apparentAge, correctedAge);
}

net::Headers validatorsFor(const QByteArray &etag, const QByteArray &lastModified)
{
    net::Headers headers;
    if (!etag.isEmpty())
        headers.append(net::Header {QByteArrayLiteral("If-None-Match"), etag});
    if (!lastModified.isEmpty())
        headers.append(net::Header {QByteArrayLiteral("If-Modified-Since"), lastModified});
    return headers;
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

ImageCache::Entry ImageCache::Entry::read(const QString &path, const QUrl &url)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {url};

    QDataStream stream(&file);
    stream.setVersion(QDataStream::Qt_6_0);
    quint32 format = 0;
    stream >> format;

    if (format != kEntryFormat) {
        file.seek(0);
        return {url, file.readAll()};
    }

    Entry entry;
    stream >> entry.url >> entry.body >> entry.etag >> entry.lastModified >> entry.expiresAt;

    if (stream.status() != QDataStream::Ok || entry.url.scheme() != url.scheme()
        || entry.url.host() != url.host())
        return {url};

    return entry;
}

QByteArray ImageCache::Entry::serialized() const
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_0);
    stream << kEntryFormat << url << body << etag << lastModified << expiresAt;
    return bytes;
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

    const bool online = net::Connectivity::instance().online();

    if (const Decoded *cached = m_memory.object(key);
        cached && (!online || cached->expiresAt > now())) {
        handler(cached->image);
        return;
    }
    if (m_pending.contains(key)) {
        m_pending[key].append({std::move(handler), std::move(needed)});
        return;
    }
    m_pending.insert(key, {{std::move(handler), std::move(needed)}});
    const QString path = m_directory + QLatin1Char('/') + key;
    Entry entry = Entry::read(path, url);

    if (!entry.body.isEmpty() && (!online || entry.expiresAt > now())) {
        const QImage image = decode(entry.body, target);

        if (!image.isNull()) {
            finish(key, image, entry.expiresAt);
            return;
        }

        QFile::remove(path);
        entry = Entry {url};
    }

    m_requests.append({key, path, target, std::move(entry)});
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
        const qint64 requestedAt = now();
        net::HttpClient::instance().get(
            request.entry.url, validatorsFor(request.entry.etag, request.entry.lastModified),
            net::Credentialed::No, [this, request, requestedAt](const net::Response &response) {
            --m_activeRequests;
            if (isNeeded(request.key))
                complete(request, response, requestedAt);
            else
                finish(request.key, {});
            startNext();
        });
    }
}

void ImageCache::complete(const Request &request, const net::Response &response, qint64 requestedAt)
{
    const Entry &stale = request.entry;
    const bool notModified = response.status == 304 && !stale.body.isEmpty();
    Entry entry =
        notModified ? stale : Entry {stale.url, response.ok() ? response.body : QByteArray()};
    const QImage image = decode(entry.body, request.size);

    if (image.isNull()) {
        qCDebug(logArtwork) << "thumbnail unavailable" << response.status;

        if (stale.url.fileName().startsWith(QLatin1String("maxresdefault"))) {
            Request fallback = request;
            QString path = fallback.entry.url.path();
            path.replace(QStringLiteral("maxresdefault"), QStringLiteral("mqdefault"));
            fallback.entry.url.setPath(path);
            m_requests.append(std::move(fallback));
            return;
        }

        if (notModified)
            QFile::remove(request.path);
        finish(request.key, decode(stale.body, request.size), stale.expiresAt);
        return;
    }

    const std::optional<qint64> expiresAt = expiryOf(response, requestedAt, now());

    if (expiresAt) {
        const QByteArray etag = response.header("etag");
        const QByteArray lastModified = response.header("last-modified");
        entry.etag = etag.isEmpty() ? entry.etag : etag;
        entry.lastModified = lastModified.isEmpty() ? entry.lastModified : lastModified;
        entry.expiresAt = *expiresAt;
        store(request.path, entry);
    } else {
        QFile::remove(request.path);
        m_memory.remove(request.key);
    }

    finish(request.key, image, expiresAt);
}

bool ImageCache::isNeeded(const QString &key) const
{
    const auto entry = m_pending.constFind(key);
    return entry != m_pending.cend()
        && std::any_of(entry->cbegin(), entry->cend(), [](const Listener &listener) {
        return !listener.needed || listener.needed();
    });
}

void ImageCache::finish(const QString &key, const QImage &image, std::optional<qint64> expiresAt)
{
    if (!image.isNull() && expiresAt)
        m_memory.insert(key, new Decoded {image, *expiresAt},
                        qMax(1, int(image.sizeInBytes() / 1024)));

    const QList<Listener> listeners = m_pending.take(key);
    for (const Listener &listener : listeners)
        listener.handler(listener.needed && !listener.needed() ? QImage() : image);
}

void ImageCache::store(const QString &path, const Entry &entry)
{
    QSaveFile file(path);

    if (file.open(QIODevice::WriteOnly)) {
        file.write(entry.serialized());
        file.commit();
    }

    if (++m_writesSincePrune >= 128) {
        m_writesSincePrune = 0;
        pruneDisk();
    }
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
