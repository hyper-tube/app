#pragma once

#include "Track.h"
#include "innertube/Endpoints.h"

#include <QList>
#include <QObject>
#include <QString>

namespace media {

class Radio : public QObject
{
    Q_OBJECT

public:
    explicit Radio(innertube::Session &session, QObject *parent = nullptr);

    bool busy() const { return m_busy; }
    void reset();
    void extend(const QString &seedVideoId);

Q_SIGNALS:
    void extended(const QList<media::Track> &tracks);

private:
    void accept(const innertube::Reply &reply, quint64 generation);

    innertube::Endpoints m_endpoints;
    QString m_seedVideoId;
    QString m_continuation;
    quint64 m_generation = 0;
    bool m_busy = false;
    bool m_exhausted = false;
};

}
