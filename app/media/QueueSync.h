#pragma once

#include "innertube/Endpoints.h"

#include <QDateTime>
#include <QObject>
#include <QString>

namespace media {

class PlaybackController;

class QueueSync : public QObject
{
    Q_OBJECT

public:
    explicit QueueSync(PlaybackController &controller, QObject *parent = nullptr);

    void start(const QDateTime &localPlayedAt);

private:
    void attempt();
    void accept(const innertube::Reply &reply);
    bool untouched() const;

    PlaybackController &m_controller;
    innertube::Endpoints m_endpoints;
    QDateTime m_localPlayedAt;
    QString m_restoredVideoId;
    qsizetype m_restoredSize = 0;
    bool m_started = false;
    bool m_requested = false;
    bool m_touched = false;
};

}
