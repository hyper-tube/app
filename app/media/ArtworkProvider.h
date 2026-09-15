#pragma once

#include <QQuickAsyncImageProvider>

namespace library {
class ImageCache;
}

namespace media {

class ArtworkProvider : public QQuickAsyncImageProvider
{
public:
    explicit ArtworkProvider(library::ImageCache &cache);

    QQuickImageResponse *requestImageResponse(const QString &id,
                                              const QSize &requestedSize) override;

private:
    library::ImageCache &m_cache;
};

}
