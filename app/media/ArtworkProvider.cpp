#include "ArtworkProvider.h"

#include "library/ImageCache.h"

#include <QImage>
#include <QQuickTextureFactory>
#include <QUrl>

#include <atomic>
#include <memory>

namespace {

class ArtworkResponse : public QQuickImageResponse
{
public:
    void cancel() override { m_needed->store(false); }

    library::ImageCache::Needed needed() const
    {
        return [needed = m_needed] { return needed->load(); };
    }

    QQuickTextureFactory *textureFactory() const override
    {
        return m_image.isNull() ? nullptr : QQuickTextureFactory::textureFactoryForImage(m_image);
    }

    void complete(const QImage &image)
    {
        m_image = image;
        if (m_image.isNull()) {
            m_image = QImage(1, 1, QImage::Format_ARGB32_Premultiplied);
            m_image.fill(Qt::transparent);
        }
        Q_EMIT finished();
    }

private:
    QImage m_image;
    std::shared_ptr<std::atomic_bool> m_needed = std::make_shared<std::atomic_bool>(true);
};

}

namespace media {

ArtworkProvider::ArtworkProvider(library::ImageCache &cache)
    : m_cache(cache)
{
}

QQuickImageResponse *ArtworkProvider::requestImageResponse(const QString &id,
                                                           const QSize &requestedSize)
{
    auto *response = new ArtworkResponse;
    const QString source = QUrl::fromPercentEncoding(id.toUtf8());
    auto *cache = &m_cache;
    QMetaObject::invokeMethod(cache, [cache, response, source, requestedSize] {
        cache->load(source, requestedSize, [response](const QImage &image) {
            response->complete(image);
        }, response->needed());
    }, Qt::QueuedConnection);
    return response;
}

}
