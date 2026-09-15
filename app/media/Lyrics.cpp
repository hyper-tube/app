#include "Lyrics.h"

#include "PlaybackController.h"
#include "core/Json.h"
#include "core/Logging.h"
#include "innertube/parsers/RendererReader.h"
#include "net/Connectivity.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>

namespace {

const QString kLyricsBrowsePrefix = QStringLiteral("MPLY");

QString lyricsBrowseId(const QJsonObject &response)
{
    const QJsonValue tabs =
        innertube::parsers::findFirst(response, QStringLiteral("watchNextTabbedResultsRenderer"));
    for (const QJsonValue &tab : core::json::at(tabs, {QStringLiteral("tabs")}).toArray()) {
        const QJsonObject renderer = core::json::object(tab, {QStringLiteral("tabRenderer")});
        const QString browseId =
            core::json::at(renderer,
                           {QStringLiteral("endpoint"), QStringLiteral("browseEndpoint"),
                            QStringLiteral("browseId")})
                .toString();
        if (browseId.startsWith(kLyricsBrowsePrefix))
            return browseId;
    }
    return {};
}

QString runsText(const QJsonValue &node)
{
    QString text;
    for (const QJsonValue &run : core::json::at(node, {QStringLiteral("runs")}).toArray())
        text += run.toObject().value(QStringLiteral("text")).toString();
    return text;
}

}

namespace media {

Lyrics::Lyrics(QObject *parent)
    : QObject(parent)
    , m_endpoints(innertube::Session::instance())
{
    PlaybackController &controller = PlaybackController::instance();
    connect(&controller, &PlaybackController::trackChanged, this, &Lyrics::refresh);
    connect(&controller, &PlaybackController::positionChanged, this,
            [this, &controller] { syncTo(controller.position()); });
    connect(&net::Connectivity::instance(), &net::Connectivity::onlineChanged, this, [this] {
        if (m_interrupted && net::Connectivity::instance().online())
            load();
    });
    refresh();
}

Lyrics &Lyrics::instance()
{
    static auto *lyrics = new Lyrics(QCoreApplication::instance());
    return *lyrics;
}

Lyrics *Lyrics::create(QQmlEngine *, QJSEngine *)
{
    QQmlEngine::setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
    return &instance();
}

void Lyrics::refresh()
{
    const QString videoId = PlaybackController::instance().track().videoId;
    if (videoId == m_videoId)
        return;

    m_videoId = videoId;
    load();
}

void Lyrics::load()
{
    discard();
    if (m_videoId.isEmpty())
        return;

    setLoading(true);
    findBrowseId(m_videoId, ++m_generation);
}

void Lyrics::retry()
{
    if (!m_interrupted)
        return;
    if (net::Connectivity::instance().online())
        load();
    else
        net::Connectivity::instance().check();
}

void Lyrics::interrupt()
{
    m_interrupted = true;
    m_loading = false;
    Q_EMIT changed();
}

void Lyrics::discard()
{
    ++m_generation;
    m_lines.clear();
    m_starts.clear();
    m_source.clear();
    m_interrupted = false;
    setActiveLine(-1);
    setLoading(false);
    Q_EMIT changed();
}

void Lyrics::findBrowseId(const QString &videoId, quint64 generation)
{
    m_endpoints.next({}, videoId, {}, [this, generation](const innertube::Reply &reply) {
        if (generation != m_generation)
            return;
        if (reply.unreachable) {
            interrupt();
            return;
        }
        const QString browseId = reply.ok() ? lyricsBrowseId(reply.json) : QString();
        if (browseId.isEmpty()) {
            setLoading(false);
            return;
        }
        fetchTimed(browseId, generation);
    });
}

void Lyrics::fetchTimed(const QString &browseId, quint64 generation)
{
    const innertube::Client *client = innertube::Session::instance().clients().lyricsClient();
    if (!client) {
        fetchPlain(browseId, generation);
        return;
    }

    m_endpoints.browseAs(*client, browseId,
                         [this, browseId, generation](const innertube::Reply &reply) {
        if (generation != m_generation)
            return;
        if (reply.unreachable) {
            interrupt();
            return;
        }
        if (reply.ok() && acceptTimed(reply)) {
            publish();
            return;
        }
        fetchPlain(browseId, generation);
    });
}

void Lyrics::fetchPlain(const QString &browseId, quint64 generation)
{
    m_endpoints.browse(browseId, {}, [this, generation](const innertube::Reply &reply) {
        if (generation != m_generation)
            return;
        if (reply.unreachable) {
            interrupt();
            return;
        }
        if (reply.ok() && acceptPlain(reply)) {
            publish();
            return;
        }
        setLoading(false);
    });
}

bool Lyrics::acceptTimed(const innertube::Reply &reply)
{
    const QJsonValue model =
        innertube::parsers::findFirst(reply.json, QStringLiteral("timedLyricsModel"));
    const QJsonValue data = core::json::at(model, {QStringLiteral("lyricsData")});
    const QJsonArray timed = core::json::at(data, {QStringLiteral("timedLyricsData")}).toArray();
    if (timed.isEmpty())
        return false;

    for (const QJsonValue &entry : timed) {
        const QJsonObject line = entry.toObject();
        const QJsonValue range = line.value(QStringLiteral("cueRange"));
        m_lines.append(line.value(QStringLiteral("lyricLine")).toString());
        m_starts.append(
            core::json::toInt(core::json::at(range, {QStringLiteral("startTimeMilliseconds")})));
    }
    m_source = core::json::at(data, {QStringLiteral("sourceMessage")}).toString();
    return true;
}

bool Lyrics::acceptPlain(const innertube::Reply &reply)
{
    const QJsonValue shelf =
        innertube::parsers::findFirst(reply.json, QStringLiteral("musicDescriptionShelfRenderer"));
    const QString body = runsText(core::json::at(shelf, {QStringLiteral("description")}));
    if (body.isEmpty())
        return false;

    m_lines = body.split(QLatin1Char('\n'));
    m_source = runsText(core::json::at(shelf, {QStringLiteral("footer")}));
    return true;
}

void Lyrics::publish()
{
    setLoading(false);
    qCDebug(logInnerTube) << "lyrics" << m_videoId << m_lines.size() << "lines"
                          << (synced() ? "synced" : "plain");
    Q_EMIT changed();
    syncTo(PlaybackController::instance().position());
}

void Lyrics::syncTo(qint64 milliseconds)
{
    if (m_starts.isEmpty()) {
        setActiveLine(-1);
        return;
    }

    int line = -1;
    for (int index = 0; index < m_starts.size() && m_starts.at(index) <= milliseconds; ++index)
        line = index;
    setActiveLine(line);
}

void Lyrics::setLoading(bool loading)
{
    if (m_loading == loading)
        return;
    m_loading = loading;
    Q_EMIT changed();
}

void Lyrics::setActiveLine(int line)
{
    if (m_activeLine == line)
        return;
    m_activeLine = line;
    Q_EMIT activeLineChanged();
}

}
