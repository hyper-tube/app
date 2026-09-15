#include "StreamResolver.h"

#include "FormatPicker.h"
#include "LiveManifest.h"
#include "core/Json.h"
#include "core/Logging.h"
#include "net/CookieJar.h"
#include "net/HttpClient.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkCookie>
#include <QStringList>
#include <QTimer>

namespace {

constexpr double kTargetLoudnessLufs = -7.0;
constexpr double kLoudnessReferenceOffset = 14.0;
constexpr double kMinimumAttenuationDb = -0.05;
constexpr double kMaximumAttenuationDb = -24.0;
constexpr qint64 kExpiryMarginSeconds = 300;
constexpr int kMaximumPictureHeight = 720;

const QString kUploadsPlaylistId = QStringLiteral("MLPT");

bool isMusicVideoType(const QString &type)
{
    return type == QLatin1String("MUSIC_VIDEO_TYPE_OMV")
        || type == QLatin1String("MUSIC_VIDEO_TYPE_UGC")
        || type == QLatin1String("MUSIC_VIDEO_TYPE_PODCAST_EPISODE");
}

std::optional<double> attenuationFor(std::optional<double> loudnessDb)
{
    if (!loudnessDb)
        return std::nullopt;

    const double gain = kTargetLoudnessLufs - (*loudnessDb - kLoudnessReferenceOffset);
    if (gain >= kMinimumAttenuationDb)
        return std::nullopt;
    return std::max(gain, kMaximumAttenuationDb);
}

QString playabilityFailure(const QJsonObject &response)
{
    const QJsonObject status = response.value(QStringLiteral("playabilityStatus")).toObject();
    const QString state = status.value(QStringLiteral("status")).toString();
    if (state == QLatin1String("OK"))
        return {};

    const QString reason = status.value(QStringLiteral("reason")).toString();
    return reason.isEmpty() ? state : state + QStringLiteral(": ") + reason;
}

QString requestCookie()
{
    const QUrl origin(QStringLiteral("https://music.youtube.com/"));
    QStringList pairs;
    for (const QNetworkCookie &cookie :
         net::HttpClient::instance().cookies().cookiesForUrl(origin)) {
        pairs.append(QString::fromUtf8(cookie.name()) + QLatin1Char('=')
                     + QString::fromUtf8(cookie.value()));
    }
    return pairs.join(QStringLiteral("; "));
}

qint64 configuredStart(const QJsonObject &response)
{
    const QJsonValue value =
        core::json::at(response,
                       {QStringLiteral("playerConfig"), QStringLiteral("playbackStartConfig"),
                        QStringLiteral("startSeconds")});
    return std::max(qint64(0), core::json::toInt(value)) * 1000;
}

std::optional<double> configuredLoudness(const QJsonObject &response)
{
    const QJsonValue value =
        core::json::at(response,
                       {QStringLiteral("playerConfig"), QStringLiteral("audioConfig"),
                        QStringLiteral("loudnessDb")});
    if (value.isUndefined() || value.isNull())
        return std::nullopt;
    return core::json::toDouble(value);
}

}

namespace player {

struct StreamResolver::Candidate
{
    Stream stream;
    qint64 expiresInSeconds = 0;
};

struct StreamResolver::Attempt
{
    QString videoId;
    quint64 generation = 0;
    bool upload = false;
    std::optional<int> signatureTimestamp;
    std::optional<Candidate> spare;
    int index = 0;
    QString title;
    QString artist;
    PlaybackTrackingSeed tracking;
    qint64 durationMs = 0;
    qint64 startMs = 0;
    std::optional<double> loudnessDb;
    std::optional<bool> musicVideo;
    std::optional<bool> episode;
    bool live = false;
};

StreamResolver::StreamResolver(innertube::Session &session, QObject *parent)
    : QObject(parent)
    , m_session(session)
    , m_endpoints(session)
{
}

void StreamResolver::setBackground(std::function<bool()> ready)
{
    m_ready = std::move(ready);
    m_endpoints.setBackground(true);
}

void StreamResolver::resolve(const QString &videoId, bool upload, bool fresh)
{
    ++m_generation;

    if (!fresh && m_cached.videoId == videoId && !m_cacheExpiry.hasExpired()) {
        qCDebug(logStream) << "reusing cached stream for" << videoId;
        Q_EMIT resolved(m_cached);
        return;
    }

    const auto attempt = std::make_shared<Attempt>();
    attempt->videoId = videoId;
    attempt->generation = m_generation;
    attempt->upload = upload;
    fetchMetadata(attempt);
}

const QStringList &StreamResolver::chainFor(const AttemptPtr &attempt) const
{
    const innertube::ClientRegistry &clients = m_session.clients();
    return attempt->upload ? clients.uploadChain() : clients.streamChain();
}

QString StreamResolver::playlistFor(const AttemptPtr &attempt)
{
    return attempt->upload ? kUploadsPlaylistId : QString();
}

void StreamResolver::fetchMetadata(const AttemptPtr &attempt)
{
    if (attempt->generation != m_generation)
        return;
    if (m_ready && !m_ready()) {
        QTimer::singleShot(250, this, [this, attempt] { fetchMetadata(attempt); });
        return;
    }

    const innertube::Client *client = m_session.clients().metadataClient();
    if (!client) {
        tryNextClient(attempt);
        return;
    }

    if (attempt->upload && !attempt->signatureTimestamp) {
        PlayerScript::instance().ready([this, attempt](int timestamp) {
            if (attempt->generation != m_generation)
                return;
            attempt->signatureTimestamp = timestamp;
            fetchMetadata(attempt);
        });
        return;
    }

    m_endpoints.player(*client, attempt->videoId, playlistFor(attempt),
                       attempt->signatureTimestamp.value_or(0),
                       [this, attempt, client](const innertube::Reply &reply) {
        if (attempt->generation != m_generation)
            return;

        if (reply.unreachable) {
            Q_EMIT failed(attempt->videoId, reply.error, true);
            return;
        }
        if (!reply.ok()) {
            qCWarning(logStream) << "metadata client failed" << client->key << reply.error;
            tryNextClient(attempt);
            return;
        }

        readMetadata(attempt, *client, reply.json);
        if (attempt->upload || attempt->signatureTimestamp.has_value()) {
            attempt->spare = buildStream(attempt, *client, reply.json);
            if (attempt->spare) {
                signWebStream(attempt);
                return;
            }
        }
        tryNextClient(attempt);
    });
}

void StreamResolver::signWebStream(const AttemptPtr &attempt)
{
    if (!attempt->spare)
        return;

    const QUrl url = PlayerScript::instance().descramble(attempt->spare->stream.url);
    if (url.isEmpty()) {
        qCWarning(logStream) << "could not sign the web stream for" << attempt->videoId;
        attempt->spare.reset();
        tryNextClient(attempt);
        return;
    }
    attempt->spare->stream.url = url;
    publish(attempt->spare->stream, attempt->spare->expiresInSeconds);
}

void StreamResolver::tryNextClient(const AttemptPtr &attempt)
{
    if (attempt->generation != m_generation)
        return;
    if (m_ready && !m_ready()) {
        QTimer::singleShot(250, this, [this, attempt] { tryNextClient(attempt); });
        return;
    }

    const QStringList &chain = chainFor(attempt);
    if (attempt->index >= chain.size()) {
        if (!attempt->upload && !attempt->signatureTimestamp) {
            PlayerScript::instance().ready([this, attempt](int timestamp) {
                if (attempt->generation != m_generation)
                    return;
                attempt->signatureTimestamp = timestamp;
                if (timestamp > 0)
                    fetchMetadata(attempt);
                else
                    tryNextClient(attempt);
            });
            return;
        }
        qCWarning(logStream) << "no client could stream" << attempt->videoId;
        Q_EMIT failed(attempt->videoId, QStringLiteral("no playable stream found"), false);
        return;
    }

    const QString key = chain.at(attempt->index++);
    const innertube::Client *client = m_session.clients().client(key);
    if (!client) {
        tryNextClient(attempt);
        return;
    }

    m_endpoints.player(*client, attempt->videoId, playlistFor(attempt), 0,
                       [this, attempt, client](const innertube::Reply &reply) {
        if (attempt->generation != m_generation)
            return;

        if (reply.unreachable) {
            Q_EMIT failed(attempt->videoId, reply.error, true);
            return;
        }
        if (!reply.ok()) {
            qCWarning(logStream) << client->key << "failed" << reply.error;
            tryNextClient(attempt);
            return;
        }

        const QString failure = playabilityFailure(reply.json);
        if (!failure.isEmpty()) {
            qCWarning(logStream) << client->key << "cannot play" << failure;
            tryNextClient(attempt);
            return;
        }

        readMetadata(attempt, *client, reply.json);
        if (attempt->live)
            takeLiveStream(attempt, *client, reply.json);
        else if (!takeStream(attempt, *client, reply.json))
            tryNextClient(attempt);
    });
}

std::optional<StreamResolver::Candidate>
StreamResolver::buildStream(const AttemptPtr &attempt, const innertube::Client &client,
                            const QJsonObject &response) const
{
    const QJsonObject streamingData = response.value(QStringLiteral("streamingData")).toObject();
    const AudioFormat format =
        formatPicker::best(streamingData.value(QStringLiteral("adaptiveFormats")).toArray());
    if (!format.valid())
        return std::nullopt;

    Candidate candidate;
    Stream &stream = candidate.stream;
    stream.videoId = attempt->videoId;
    stream.url = format.url;
    stream.userAgent = client.userAgent;
    if (client.loginSupported)
        stream.cookie = requestCookie();
    stream.clientKey = client.key;
    stream.mimeType = format.mimeType;
    stream.title = attempt->title;
    stream.artist = attempt->artist;
    stream.tracking = attempt->tracking;
    stream.durationMs = attempt->durationMs;
    stream.startMs = attempt->startMs;
    stream.episode = attempt->episode;
    stream.itag = format.itag;
    stream.musicVideo = attempt->musicVideo;

    const VideoFormat picture = formatPicker::bestPicture(
        streamingData.value(QStringLiteral("adaptiveFormats")).toArray(), kMaximumPictureHeight);
    if (picture.valid()) {
        stream.pictureUrl = picture.url;
        stream.pictureItag = picture.itag;
        stream.pictureHeight = picture.height;
    }

    stream.gainDb =
        attenuationFor(format.hasLoudness ? std::optional(format.loudnessDb) : attempt->loudnessDb);
    candidate.expiresInSeconds =
        core::json::toInt(streamingData.value(QStringLiteral("expiresInSeconds")));
    return candidate;
}

bool StreamResolver::takeStream(const AttemptPtr &attempt, const innertube::Client &client,
                                const QJsonObject &response)
{
    const std::optional<Candidate> candidate = buildStream(attempt, client, response);
    if (!candidate)
        return false;
    publish(candidate->stream, candidate->expiresInSeconds);
    return true;
}

void StreamResolver::takeLiveStream(const AttemptPtr &attempt, const innertube::Client &client,
                                    const QJsonObject &response)
{
    const QJsonObject streamingData = response.value(QStringLiteral("streamingData")).toObject();
    const QUrl manifest(streamingData.value(QStringLiteral("hlsManifestUrl")).toString());
    if (manifest.isEmpty()) {
        qCWarning(logStream) << client.key << "offered no live manifest for" << attempt->videoId;
        tryNextClient(attempt);
        return;
    }

    const qint64 expiresInSeconds =
        core::json::toInt(streamingData.value(QStringLiteral("expiresInSeconds")));
    const net::Headers headers {{QByteArrayLiteral("User-Agent"), client.userAgent.toUtf8()}};
    net::HttpClient::instance().get(
        manifest, headers, net::Credentialed::No,
        [this, attempt, manifest, expiresInSeconds, client = &client](const net::Response &reply) {
        if (attempt->generation != m_generation)
            return;
        if (reply.unreachable) {
            Q_EMIT failed(attempt->videoId, reply.error, true);
            return;
        }
        const LiveManifest renditions = reply.ok()
            ? LiveManifest::parse(reply.body, manifest, kMaximumPictureHeight)
            : LiveManifest();
        if (!renditions.valid()) {
            qCWarning(logStream) << client->key << "live manifest unusable" << reply.error;
            tryNextClient(attempt);
            return;
        }

        Stream stream;
        stream.videoId = attempt->videoId;
        stream.url = renditions.audio;
        stream.pictureUrl = renditions.picture;
        stream.pictureHeight = renditions.pictureHeight;
        stream.userAgent = client->userAgent;
        stream.clientKey = client->key;
        stream.mimeType = QStringLiteral("application/x-mpegURL");
        stream.title = attempt->title;
        stream.artist = attempt->artist;
        stream.tracking = attempt->tracking;
        stream.musicVideo = attempt->musicVideo;
        stream.live = true;
        publish(stream, expiresInSeconds);
    });
}

void StreamResolver::readMetadata(const AttemptPtr &attempt, const innertube::Client &client,
                                  const QJsonObject &response)
{
    const QJsonObject details = response.value(QStringLiteral("videoDetails")).toObject();

    if (attempt->title.isEmpty())
        attempt->title = details.value(QStringLiteral("title")).toString();
    if (attempt->artist.isEmpty())
        attempt->artist = details.value(QStringLiteral("author")).toString();
    if (details.value(QStringLiteral("isLive")).toBool())
        attempt->live = true;
    if (attempt->durationMs == 0 && !attempt->live)
        attempt->durationMs =
            core::json::toInt(details.value(QStringLiteral("lengthSeconds"))) * 1000;
    if (!attempt->loudnessDb)
        attempt->loudnessDb = configuredLoudness(response);
    if (attempt->startMs == 0)
        attempt->startMs = configuredStart(response);
    const QString videoType = details.value(QStringLiteral("musicVideoType")).toString();
    if (!attempt->musicVideo && !videoType.isEmpty())
        attempt->musicVideo = isMusicVideoType(videoType);
    if (!attempt->episode && !videoType.isEmpty())
        attempt->episode = videoType == QLatin1String("MUSIC_VIDEO_TYPE_PODCAST_EPISODE");
    if (attempt->tracking.playbackUrl.isEmpty())
        attempt->tracking = PlaybackTrackingSeed::fromResponse(response, client.key, client.name);
}

void StreamResolver::publish(const Stream &stream, qint64 expiresInSeconds)
{
    const qint64 lifetime = std::max(qint64(0), expiresInSeconds - kExpiryMarginSeconds);
    m_cached = stream;
    m_cacheExpiry = QDeadlineTimer(lifetime * 1000);

    qCInfo(logStream) << "resolved" << stream.videoId << "via" << stream.clientKey << "itag"
                      << stream.itag << "picture" << stream.pictureItag << "for" << lifetime << "s";
    Q_EMIT resolved(stream);
}

}
