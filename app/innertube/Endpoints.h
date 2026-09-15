#pragma once

#include "Session.h"

#include <QString>

namespace innertube {

class Endpoints
{
public:
    explicit Endpoints(Session &session);

    void setBackground(bool background) { m_background = background; }
    void player(const Client &client, const QString &videoId, const QString &playlistId,
                int signatureTimestamp, const Session::Handler &handler);
    void browse(const QString &browseId, const QString &params, const Session::Handler &handler);
    void browseAs(const Client &client, const QString &browseId, const Session::Handler &handler);
    void search(const QString &query, const QString &params, const Session::Handler &handler);
    void next(const QString &playlistId, const QString &videoId, const QString &params,
              const Session::Handler &handler);
    void continuation(const QString &endpoint, const QString &token,
                      const Session::Handler &handler);
    void accountMenu(const Session::Handler &handler);
    void action(const QString &endpoint, const QJsonObject &body, const Session::Handler &handler);

private:
    void metadata(const QString &endpoint, const QJsonObject &body,
                  const Session::Handler &handler);
    Session &m_session;
    bool m_background = false;
};

}
