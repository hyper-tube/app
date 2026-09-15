#pragma once

#include "PlayerScript.h"
#include "PlaybackTrackingSeed.h"
#include "innertube/Endpoints.h"

#include <QDeadlineTimer>
#include <QObject>
#include <QString>
#include <QUrl>

#include <memory>
#include <optional>

namespace player {

struct Stream
{
    QString videoId;
    QUrl url;
    QUrl pictureUrl;
    QString userAgent;
    QString cookie;
    QString clientKey;
    QString mimeType;
    QString title;
    QString artist;
    PlaybackTrackingSeed tracking;
    qint64 durationMs = 0;
    qint64 startMs = 0;
    int itag = 0;
    int pictureItag = 0;
    int pictureHeight = 0;
    std::optional<double> gainDb;
    std::optional<bool> musicVideo;
    std::optional<bool> episode;
    bool live = false;

    bool valid() const { return !url.isEmpty(); }
    bool showsPicture() const { return !pictureUrl.isEmpty(); }
};

class StreamResolver : public QObject
{
    Q_OBJECT

public:
    explicit StreamResolver(innertube::Session &session, QObject *parent = nullptr);

    void setBackground(std::function<bool()> ready);
    void resolve(const QString &videoId, bool upload, bool fresh = false);

Q_SIGNALS:
    void resolved(const player::Stream &stream);
    void failed(const QString &videoId, const QString &message, bool unreachable);

private:
    struct Attempt;
    struct Candidate;
    using AttemptPtr = std::shared_ptr<Attempt>;

    void fetchMetadata(const AttemptPtr &attempt);
    void signWebStream(const AttemptPtr &attempt);
    void tryNextClient(const AttemptPtr &attempt);
    const QStringList &chainFor(const AttemptPtr &attempt) const;
    static QString playlistFor(const AttemptPtr &attempt);
    std::optional<Candidate> buildStream(const AttemptPtr &attempt, const innertube::Client &client,
                                         const QJsonObject &response) const;
    bool takeStream(const AttemptPtr &attempt, const innertube::Client &client,
                    const QJsonObject &response);
    void takeLiveStream(const AttemptPtr &attempt, const innertube::Client &client,
                        const QJsonObject &response);
    void readMetadata(const AttemptPtr &attempt, const innertube::Client &client,
                      const QJsonObject &response);
    void publish(const Stream &stream, qint64 expiresInSeconds);

    innertube::Session &m_session;
    innertube::Endpoints m_endpoints;
    std::function<bool()> m_ready;
    quint64 m_generation = 0;
    Stream m_cached;
    QDeadlineTimer m_cacheExpiry;
};

}
