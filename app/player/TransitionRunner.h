#pragma once

#include "TransitionPlan.h"

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

namespace player {

class MpvController;

class TransitionRunner : public QObject
{
    Q_OBJECT

public:
    explicit TransitionRunner(QObject *parent = nullptr);

    bool active() const { return m_outgoing != nullptr; }
    bool started() const { return m_incomingStarted; }
    bool promoted() const { return m_promoted; }
    bool finishing() const { return m_finishing; }

    bool prepare(MpvController &outgoing, MpvController &incoming, const TransitionPlan &plan);
    void incomingLoaded(MpvController &deck);
    void confirmPromotion();
    void finish(bool requestPromotion);
    void abort();

Q_SIGNALS:
    void promotionRequested();
    void fallbackRequested();

private:
    void update();
    void stepShelves(double outgoingPts, double incomingPts);
    void lock(double outgoingPts, double incomingPts);
    void returnSpeed(double incomingPts);
    void stopOutgoing();
    void requestFallback();
    void clear();

    MpvController *m_outgoing = nullptr;
    MpvController *m_incoming = nullptr;
    TransitionPlan m_plan;
    QTimer m_timer;
    QElapsedTimer m_finishClock;
    double m_finishSpeed = 1.0;
    double m_finishStartPts = 0.0;
    bool m_incomingStarted = false;
    bool m_locked = false;
    bool m_outgoingStopped = false;
    bool m_promoted = false;
    bool m_promotionRequested = false;
    bool m_returning = false;
    bool m_finishing = false;
    bool m_lockErrorSeeded = false;
    double m_lastLockError = 0.0;
    double m_lockError = 0.0;
};

}
