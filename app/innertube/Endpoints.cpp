#include "Endpoints.h"

#include <QJsonObject>
#include <QUrlQuery>

namespace {

const QString kPlayerEndpoint = QStringLiteral("player");

}

namespace innertube {

Endpoints::Endpoints(Session &session)
    : m_session(session)
{
}

void Endpoints::player(const Client &client, const QString &videoId, const QString &playlistId,
                       int signatureTimestamp, const Session::Handler &handler)
{
    QJsonObject body;
    body.insert(QStringLiteral("videoId"), videoId);
    body.insert(QStringLiteral("contentCheckOk"), true);
    body.insert(QStringLiteral("racyCheckOk"), true);
    if (!playlistId.isEmpty())
        body.insert(QStringLiteral("playlistId"), playlistId);
    if (signatureTimestamp > 0) {
        body.insert(QStringLiteral("playbackContext"),
                    QJsonObject {{QStringLiteral("contentPlaybackContext"),
                                  QJsonObject {{QStringLiteral("html5Preference"),
                                                QStringLiteral("HTML5_PREF_WANTS")},
                                               {QStringLiteral("signatureTimestamp"),
                                                signatureTimestamp}}}});
    }

    m_session.call(kPlayerEndpoint, client, body, handler, m_background);
}

void Endpoints::browse(const QString &browseId, const QString &params,
                       const Session::Handler &handler)
{
    QJsonObject body {{QStringLiteral("browseId"), browseId}};
    if (!params.isEmpty())
        body.insert(QStringLiteral("params"), params);
    metadata(QStringLiteral("browse"), body, handler);
}

void Endpoints::browseAs(const Client &client, const QString &browseId,
                         const Session::Handler &handler)
{
    m_session.call(QStringLiteral("browse"), client, {{QStringLiteral("browseId"), browseId}},
                   handler);
}

void Endpoints::search(const QString &query, const QString &params, const Session::Handler &handler)
{
    QJsonObject body {{QStringLiteral("query"), query}};
    if (!params.isEmpty())
        body.insert(QStringLiteral("params"), params);
    metadata(QStringLiteral("search"), body, handler);
}

void Endpoints::next(const QString &playlistId, const QString &videoId, const QString &params,
                     const Session::Handler &handler)
{
    QJsonObject body {{QStringLiteral("isAudioOnly"), true}};
    if (!playlistId.isEmpty())
        body.insert(QStringLiteral("playlistId"), playlistId);
    if (!videoId.isEmpty())
        body.insert(QStringLiteral("videoId"), videoId);
    if (!params.isEmpty())
        body.insert(QStringLiteral("params"), params);
    metadata(QStringLiteral("next"), body, handler);
}

void Endpoints::continuation(const QString &endpoint, const QString &token,
                             const Session::Handler &handler)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("continuation"), token);
    query.addQueryItem(QStringLiteral("ctoken"), token);
    metadata(endpoint + QLatin1Char('?') + query.toString(QUrl::FullyEncoded),
             {{QStringLiteral("continuation"), token}}, handler);
}

void Endpoints::accountMenu(const Session::Handler &handler)
{
    metadata(QStringLiteral("account/account_menu"), {}, handler);
}

void Endpoints::action(const QString &endpoint, const QJsonObject &body,
                       const Session::Handler &handler)
{
    metadata(endpoint, body, handler);
}

void Endpoints::metadata(const QString &endpoint, const QJsonObject &body,
                         const Session::Handler &handler)
{
    const Client *client = m_session.clients().metadataClient();
    if (!client) {
        handler({{}, QStringLiteral("Metadata client is unavailable")});
        return;
    }
    m_session.call(endpoint, *client, body, handler, m_background);
}

}
