#include "QueueSync.h"

#include "PlaybackController.h"
#include "auth/Account.h"
#include "core/Json.h"
#include "core/Logging.h"
#include "innertube/parsers/RendererParser.h"
#include "innertube/parsers/RendererReader.h"
#include "net/Connectivity.h"

#include <QJsonObject>

namespace {

using innertube::parsers::findFirst;

const QString kTrackRadioPrefix = QStringLiteral("RDAMVM");

struct RemoteQueue
{
    QList<media::Track> tracks;
    model::Item source;
    QDateTime watchedAt;
    int index = 0;
};

RemoteQueue readQueue(const QJsonObject &response)
{
    RemoteQueue queue;
    const QJsonObject renderer =
        findFirst(response.value(QStringLiteral("contents")), QStringLiteral("musicQueueRenderer"))
            .toObject();
    const innertube::parsers::Page page = innertube::parsers::RendererParser::parse(
        {{QStringLiteral("content"), renderer.value(QStringLiteral("content"))}});
    for (const model::Shelf &shelf : page.shelves) {
        for (const model::Item &item : shelf.items) {
            if (item.playable())
                queue.tracks.append(item.track);
        }
    }
    if (queue.tracks.isEmpty())
        return queue;

    const QJsonObject watch = findFirst(response.value(QStringLiteral("currentVideoEndpoint")),
                                        QStringLiteral("watchEndpoint"))
                                  .toObject();
    const QString current = watch.value(QStringLiteral("videoId")).toString();
    queue.index = int(qBound(qint64(0), core::json::toInt(watch.value(QStringLiteral("index"))),
                             qint64(queue.tracks.size() - 1)));
    for (int position = 0; position < queue.tracks.size(); ++position) {
        if (queue.tracks.at(position).videoId == current) {
            queue.index = position;
            break;
        }
    }

    const QJsonValue seconds =
        findFirst(renderer.value(QStringLiteral("musicQueueConfig")), QStringLiteral("seconds"));
    if (!seconds.isUndefined())
        queue.watchedAt = QDateTime::fromSecsSinceEpoch(core::json::toInt(seconds));

    queue.source.playlistId = watch.value(QStringLiteral("playlistId")).toString();
    queue.source.title = innertube::parsers::readText(
        findFirst(renderer.value(QStringLiteral("header")), QStringLiteral("subtitle")));
    queue.source.kind = queue.source.playlistId.startsWith(kTrackRadioPrefix)
        ? QStringLiteral("mix")
        : QStringLiteral("playlist");
    if (!queue.source.valid())
        queue.source = {};
    return queue;
}

}

namespace media {

QueueSync::QueueSync(PlaybackController &controller, QObject *parent)
    : QObject(parent)
    , m_controller(controller)
    , m_endpoints(innertube::Session::instance())
{
    connect(&auth::Account::instance(), &auth::Account::changed, this, &QueueSync::attempt);
    connect(&net::Connectivity::instance(), &net::Connectivity::onlineChanged, this,
            &QueueSync::attempt);
    connect(&m_controller, &PlaybackController::playingChanged, this, [this] {
        if (m_controller.playing())
            m_touched = true;
    });
}

void QueueSync::start(const QDateTime &localPlayedAt)
{
    m_started = true;
    m_localPlayedAt = localPlayedAt;
    m_restoredVideoId = m_controller.track().videoId;
    m_restoredSize = m_controller.queue().size();
    attempt();
}

bool QueueSync::untouched() const
{
    return !m_touched && m_controller.track().videoId == m_restoredVideoId
        && m_controller.queue().size() == m_restoredSize;
}

void QueueSync::attempt()
{
    if (!m_started || m_requested || !untouched() || !auth::Account::instance().signedIn()
        || !net::Connectivity::instance().online())
        return;
    m_requested = true;
    const QJsonObject body {
        {QStringLiteral("watchNextType"), QStringLiteral("WATCH_NEXT_TYPE_GET_QUEUE")},
    };
    m_endpoints.action(QStringLiteral("next"), body,
                       [this](const innertube::Reply &reply) { accept(reply); });
}

void QueueSync::accept(const innertube::Reply &reply)
{
    if (!reply.ok()) {
        qCDebug(logPlayback) << "no server queue" << reply.error;
        return;
    }
    const RemoteQueue remote = readQueue(reply.json);
    if (remote.tracks.isEmpty() || !untouched())
        return;

    const QString &current = remote.tracks.at(remote.index).videoId;
    if (current == m_restoredVideoId)
        return;
    if (!m_restoredVideoId.isEmpty()
        && (!remote.watchedAt.isValid()
            || (m_localPlayedAt.isValid() && remote.watchedAt <= m_localPlayedAt))) {
        qCInfo(logPlayback) << "keeping the local queue over the server queue from"
                            << remote.watchedAt;
        return;
    }

    qCInfo(logPlayback) << "continuing the server queue at" << current << "from"
                        << remote.watchedAt;
    m_controller.continueRemoteQueue(remote.tracks, remote.index, remote.source);
}

}
