#pragma once

#include "MpvController.h"
#include "StreamResolver.h"
#include "TransitionPlan.h"
#include "TransitionRunner.h"

#include <QObject>
#include <QString>

#include <array>
#include <optional>

namespace player {

class AudioEngine : public QObject
{
    Q_OBJECT

public:
    explicit AudioEngine(QObject *parent = nullptr);

    bool playing() const;
    int volume() const { return m_volume; }
    bool muted() const { return m_muted; }
    const QString &stagedVideoId() const { return m_stagedVideoId; }
    bool staged() const;
    bool fading() const;
    bool transitionRunning() const { return m_runner.active(); }
    bool transitionStarted() const { return m_runner.started(); }
    bool transitionPromoted() const { return m_runner.promoted(); }
    bool hasPicture() const;
    int pictureWidth() const;
    int pictureHeight() const;

    void load(const Stream &stream, qint64 startMilliseconds);
    void stage(const Stream &stream);
    bool stage(const Stream &stream, const TransitionPlan &plan);
    void fallbackToPhaseTwo(const Stream &stream);
    void discardStaged();
    void endTransition();
    std::optional<qint64> scheduleCrossfade(qint64 fadeMilliseconds, qint64 audibleEndMilliseconds,
                                            qint64 incomingStartMilliseconds);
    std::optional<qint64> scheduleSmartTransition(qint64 fadeStartMilliseconds,
                                                  qint64 stopMilliseconds,
                                                  qint64 incomingStartMilliseconds,
                                                  bool naturalFade);
    void promoteStaged(bool crossfade);
    void play();
    void pause();
    void stop();
    void seek(qint64 milliseconds);
    void setLoopFile(bool loop);
    void setVolume(int volume);
    void setMuted(bool muted);
    void setRate(double rate);
    void setPicture(const QUrl &url);

Q_SIGNALS:
    void positionChanged(qint64 milliseconds);
    void durationChanged(qint64 milliseconds);
    void playingChanged(bool playing);
    void trackEnded();
    void discontinuity();
    void ending();
    void failed(const QString &message);
    void stagedChanged();
    void pictureChanged();
    void transitionPromotionRequested();
    void transitionFallbackRequested();

private:
    MpvController &active();
    MpvController &idle();
    const MpvController &active() const { return m_decks.at(m_active); }
    const MpvController &idle() const { return m_decks.at(1 - m_active); }
    void adopt(MpvController &deck) const;
    void routePicture();
    void cancelCrossfade();
    void endFade();

    std::array<MpvController, 2> m_decks;
    TransitionRunner m_runner;
    QString m_stagedVideoId;
    std::optional<qint64> m_handoffMs;
    std::optional<qint64> m_outgoingStopMs;
    int m_active = 0;
    int m_retiring = -1;
    int m_volume = 100;
    double m_rate = 1.0;
    bool m_muted = false;
    bool m_loop = false;
};

}
