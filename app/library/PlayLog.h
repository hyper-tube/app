#pragma once

#include "media/Track.h"

#include <QHash>
#include <QObject>

namespace library {

class PlayLog : public QObject
{
    Q_OBJECT

public:
    struct Entry
    {
        media::Track track;
        int plays = 0;
        int skips = 0;
        double completion = 0;
        qint64 lastPlayed = 0;
    };

    explicit PlayLog(QObject *parent = nullptr);

    static PlayLog &instance();
    const QHash<QString, Entry> &entries() const { return m_entries; }
    void begin(const media::Track &track);
    void reach(qint64 position, qint64 duration);
    void finish();

private:
    void load();
    void save() const;
    void setRating(const QString &videoId, int rating);

    QHash<QString, Entry> m_entries;
    media::Track m_track;
    double m_reached = 0;
    bool m_started = false;
};

}
