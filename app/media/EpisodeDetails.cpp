#include "EpisodeDetails.h"

#include "PlaybackController.h"
#include "core/Logging.h"
#include "innertube/parsers/RendererReader.h"
#include "net/Connectivity.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QJsonArray>
#include <QPointer>
#include <QUrl>

namespace {

const QString kDetailsPrefix = QStringLiteral("MPPE");
const QString kSeekScheme = QStringLiteral("seek");

QString escaped(const QString &text)
{
    return text.toHtmlEscaped().replace(QLatin1Char('\n'), QStringLiteral("<br>"));
}

QString anchor(const QString &href, const QString &text)
{
    return QStringLiteral("<a href=\"%1\">%2</a>").arg(href.toHtmlEscaped(), escaped(text));
}

QString richDescription(const QJsonValue &description, const QString &videoId)
{
    using innertube::parsers::findFirst;
    QString html;
    for (const QJsonValue &value : description.toObject().value(QStringLiteral("runs")).toArray()) {
        const QJsonObject run = value.toObject();
        const QString text = run.value(QStringLiteral("text")).toString();
        const QJsonValue endpoint = run.value(QStringLiteral("navigationEndpoint"));
        const QJsonObject watch = findFirst(endpoint, QStringLiteral("watchEndpoint")).toObject();
        const QUrl url(findFirst(endpoint, QStringLiteral("url")).toString());
        if (!watch.isEmpty() && watch.value(QStringLiteral("videoId")).toString() == videoId) {
            const int seconds = watch.value(QStringLiteral("startTimeSeconds")).toInt();
            html += anchor(kSeekScheme + QLatin1Char(':') + QString::number(seconds), text);
        } else if (url.isValid()
                   && (url.scheme() == QLatin1String("https")
                       || url.scheme() == QLatin1String("http"))) {
            html += anchor(url.toString(QUrl::FullyEncoded), text);
        } else {
            html += escaped(text);
        }
    }
    return html;
}

}

namespace media {

EpisodeDetails::EpisodeDetails(QObject *parent)
    : QObject(parent)
    , m_endpoints(innertube::Session::instance())
{
    connect(&PlaybackController::instance(), &PlaybackController::trackChanged, this,
            &EpisodeDetails::follow);
    connect(&net::Connectivity::instance(), &net::Connectivity::onlineChanged, this, [this] {
        if (m_unreachable && net::Connectivity::instance().online())
            fetch();
    });
    follow();
}

EpisodeDetails &EpisodeDetails::instance()
{
    static auto *details = new EpisodeDetails(QCoreApplication::instance());
    return *details;
}

EpisodeDetails *EpisodeDetails::create(QQmlEngine *, QJSEngine *)
{
    QQmlEngine::setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
    return &instance();
}

void EpisodeDetails::follow()
{
    const auto &playback = PlaybackController::instance();
    const Track track = playback.track();
    if (!playback.episode()) {
        if (!m_videoId.isEmpty())
            clear();
        return;
    }
    if (track.videoId == m_videoId)
        return;
    clear();
    m_videoId = track.videoId;
    m_title = track.title;
    m_podcast = track.artist;
    m_artId = track.artId;
    fetch();
}

void EpisodeDetails::clear()
{
    ++m_generation;
    m_videoId.clear();
    m_title.clear();
    m_podcast.clear();
    m_podcastId.clear();
    m_artId.clear();
    m_meta.clear();
    m_description.clear();
    m_loading = false;
    m_failed = false;
    m_unreachable = false;
    Q_EMIT changed();
}

void EpisodeDetails::retry()
{
    if (!m_videoId.isEmpty() && !m_loading)
        fetch();
}

void EpisodeDetails::fetch()
{
    const quint64 generation = ++m_generation;
    m_loading = true;
    m_failed = false;
    m_unreachable = false;
    Q_EMIT changed();
    const QPointer<EpisodeDetails> guard(this);
    m_endpoints.browse(kDetailsPrefix + m_videoId, {},
                       [guard, generation](const innertube::Reply &reply) {
        if (guard)
            guard->accept(reply, generation);
    });
}

void EpisodeDetails::accept(const innertube::Reply &reply, quint64 generation)
{
    using innertube::parsers::findFirst;
    using innertube::parsers::readText;
    if (generation != m_generation)
        return;
    m_loading = false;
    const QJsonObject shelf =
        findFirst(reply.json, QStringLiteral("musicDescriptionShelfRenderer")).toObject();
    if (!reply.ok() || shelf.isEmpty()) {
        m_failed = true;
        m_unreachable = reply.unreachable;
        qCDebug(logInnerTube) << "episode details unavailable" << m_videoId << reply.error;
        Q_EMIT changed();
        return;
    }
    const QString header = readText(shelf.value(QStringLiteral("header")));
    if (!header.isEmpty())
        m_title = header;
    const QJsonValue subheader = shelf.value(QStringLiteral("subheader"));
    const QString podcast = readText(subheader);
    if (!podcast.isEmpty())
        m_podcast = podcast;
    m_podcastId = findFirst(subheader, QStringLiteral("browseId")).toString();
    m_meta = readText(shelf.value(QStringLiteral("strapline")));
    m_description = richDescription(shelf.value(QStringLiteral("description")), m_videoId);
    Q_EMIT changed();
}

void EpisodeDetails::activate(const QString &link) const
{
    const QUrl url(link);
    if (url.scheme() == kSeekScheme) {
        auto &playback = PlaybackController::instance();
        if (playback.track().videoId == m_videoId)
            playback.seek(url.path().toLongLong() * 1000);
        return;
    }
    if (url.scheme() == QLatin1String("https") || url.scheme() == QLatin1String("http"))
        QDesktopServices::openUrl(url);
}

}
