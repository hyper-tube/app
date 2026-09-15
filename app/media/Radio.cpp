#include "Radio.h"

#include "core/Logging.h"
#include "innertube/parsers/RendererParser.h"
#include "innertube/parsers/RendererReader.h"

#include <QJsonArray>
#include <QJsonObject>

namespace {

const QString kRadioPrefix = QStringLiteral("RDAMVM");
const QString kNextEndpoint = QStringLiteral("next");

QString queueContinuation(const QJsonObject &response)
{
    const QJsonValue container =
        innertube::parsers::findFirst(response, QStringLiteral("continuations"));
    for (const QJsonValue &entry : container.toArray()) {
        const QJsonObject data = entry.toObject();
        for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
            const QString token =
                it.value().toObject().value(QStringLiteral("continuation")).toString();
            if (!token.isEmpty())
                return token;
        }
    }
    return {};
}

}

namespace media {

Radio::Radio(innertube::Session &session, QObject *parent)
    : QObject(parent)
    , m_endpoints(session)
{
}

void Radio::reset()
{
    ++m_generation;
    m_seedVideoId.clear();
    m_continuation.clear();
    m_busy = false;
    m_exhausted = false;
}

void Radio::extend(const QString &seedVideoId)
{
    if (m_busy || seedVideoId.isEmpty())
        return;

    if (m_seedVideoId != seedVideoId) {
        m_seedVideoId = seedVideoId;
        m_continuation.clear();
        m_exhausted = false;
    }
    if (m_exhausted)
        return;

    m_busy = true;
    const quint64 generation = ++m_generation;
    const auto handler = [this, generation](const innertube::Reply &reply) {
        accept(reply, generation);
    };

    if (m_continuation.isEmpty())
        m_endpoints.next(kRadioPrefix + m_seedVideoId, m_seedVideoId, {}, handler);
    else
        m_endpoints.continuation(kNextEndpoint, m_continuation, handler);
}

void Radio::accept(const innertube::Reply &reply, quint64 generation)
{
    if (generation != m_generation)
        return;

    m_busy = false;
    if (!reply.ok()) {
        qCWarning(logInnerTube) << "autoplay could not be extended" << reply.error;
        m_exhausted = true;
        return;
    }

    const innertube::parsers::Page page = innertube::parsers::RendererParser::parse(reply.json);
    m_continuation = queueContinuation(reply.json);
    m_exhausted = m_continuation.isEmpty();

    QList<Track> tracks;
    for (const model::Shelf &shelf : page.shelves) {
        for (const model::Item &item : shelf.items) {
            if (item.track.valid())
                tracks.append(item.track);
        }
    }
    if (tracks.isEmpty()) {
        m_exhausted = true;
        return;
    }

    Q_EMIT extended(tracks);
}

}
