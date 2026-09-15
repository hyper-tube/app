#pragma once

#include "PlaybackTrackingSeed.h"

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

namespace player {

class PlaybackTracker : public QObject
{
    Q_OBJECT

public:
    explicit PlaybackTracker(QObject *parent = nullptr);

    void prepare(const PlaybackTrackingSeed &seed);
    void setSeed(const PlaybackTrackingSeed &seed);
    void setPlaying(bool playing);
    void observe(qint64 positionMs);
    void discontinuity();
    void finish();
    void cancel();

Q_SIGNALS:
    void pingRequested(const QString &clientKey, const QUrl &url);

private:
    void start();
    void flush(bool terminal);
    void schedule();
    QUrl requestUrl(const QUrl &base) const;

    PlaybackTrackingSeed m_seed;
    QTimer m_timer;
    QElapsedTimer m_wallClock;
    QElapsedTimer m_observationClock;
    QString m_nonce;
    qint64 m_anchor = 0;
    qint64 m_position = 0;
    bool m_playing = false;
    bool m_anchored = false;
};

}