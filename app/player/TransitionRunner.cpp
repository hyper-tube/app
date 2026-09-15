#include "TransitionRunner.h"

#include "MpvController.h"
#include "core/Logging.h"

#include <cmath>

namespace {

constexpr qint64 kSilentPrerollMs = 135;
constexpr qint64 kSpliceMs = 135;
constexpr qint64 kLockDeadlineMs = 1200;
constexpr qint64 kFinishReturnMs = 1000;
constexpr double kBassCutDb = -12.0;
constexpr double kLockGainPerSecond = 0.8;
constexpr double kMaximumLockCorrection = 0.05;
constexpr double kLockToleranceMs = 2.0;
constexpr double kLockSmoothing = 0.3;

double shelfGain(const player::Fade &fade, double position)
{
    if (!fade.active())
        return 0.0;

    const double progress = qBound(0.0, (position - fade.startMs) / double(fade.lengthMs), 1.0);
    if (fade.direction == player::Fade::Out)
        return kBassCutDb * progress;
    return kBassCutDb * (1.0 - progress);
}

}

namespace player {

TransitionRunner::TransitionRunner(QObject *parent)
    : QObject(parent)
{
    m_timer.setInterval(20);
    connect(&m_timer, &QTimer::timeout, this, &TransitionRunner::update);
}

bool TransitionRunner::prepare(MpvController &outgoing, MpvController &incoming,
                               const TransitionPlan &plan)
{
    if (!plan.valid() || plan.kind == TransitionPlan::Kind::Fixed)
        return false;

    abort();
    m_outgoing = &outgoing;
    m_incoming = &incoming;
    m_plan = plan;
    if (!m_plan.incomingFade.active()) {
        m_plan.incomingFade = {Fade::In, m_plan.incomingStartMs, kSpliceMs};
        if (!m_plan.outgoingFade.active()) {
            m_plan.outgoingFade = {Fade::Out, qMax(qint64(0), m_plan.outgoingStopMs - kSpliceMs),
                                   kSpliceMs};
        }
    }

    outgoing.setTransitionFilters(true);
    incoming.setTransitionFilters(true);
    outgoing.setFade(m_plan.outgoingFade);
    outgoing.setLowShelf(0.0);
    incoming.setFade(m_plan.incomingFade);
    incoming.setLowShelf(m_plan.incomingBassEntry.active() ? kBassCutDb : 0.0);
    if (m_plan.phaseLocked) {
        incoming.setStretcher(true);
        incoming.setSpeed(m_plan.incomingSpeed);
    } else {
        incoming.setSpeed(1.0);
    }
    return true;
}

void TransitionRunner::incomingLoaded(MpvController &deck)
{
    if (!active() || &deck != m_incoming)
        return;

    deck.seek(m_plan.incomingStartMs);
    m_timer.start();
}

void TransitionRunner::confirmPromotion()
{
    if (!active())
        return;

    m_promoted = true;
    if (!m_incoming->loaded())
        requestFallback();
}

void TransitionRunner::finish(bool requestPromotion)
{
    if (!active())
        return;

    m_finishing = true;
    stopOutgoing();
    m_incoming->clearFadeAndShelf();
    m_finishSpeed = m_incoming->speed();
    m_finishStartPts =
        m_incoming->hasAudioPts() ? m_incoming->audioPts() : double(m_incoming->position());
    m_returning = std::abs(m_finishSpeed - 1.0) >= 0.00001;
    if (m_returning) {
        m_finishClock.start();
        m_timer.start();
    }

    if (requestPromotion && !m_promoted && !m_promotionRequested && m_incoming->loaded()) {
        m_promotionRequested = true;
        Q_EMIT promotionRequested();
    }
    if (!m_returning)
        clear();
}

void TransitionRunner::abort()
{
    if (!active())
        return;

    m_timer.stop();
    m_outgoing->clearFadeAndShelf();
    if (m_promoted) {
        m_incoming->clearFadeAndShelf();
        m_incoming->setSpeed(1.0);
    } else {
        m_incoming->pause();
        m_incoming->forgetTransition();
    }
    clear();
}

void TransitionRunner::update()
{
    if (!active()) {
        m_timer.stop();
        return;
    }
    if (!m_incoming->loaded()) {
        requestFallback();
        return;
    }

    const double outgoingPts =
        m_outgoing->hasAudioPts() ? m_outgoing->audioPts() : double(m_outgoing->position());
    const double incomingPts =
        m_incoming->hasAudioPts() ? m_incoming->audioPts() : double(m_incoming->position());

    if (!m_incomingStarted) {
        const qint64 preroll = m_plan.incomingFade.active() ? kSilentPrerollMs : 0;
        if (outgoingPts >= m_plan.outgoingStartMs - preroll) {
            m_incoming->play();
            m_incomingStarted = true;
        }
    }

    if (m_finishing) {
        returnSpeed(incomingPts);
        return;
    }

    stepShelves(outgoingPts, incomingPts);
    if (m_incomingStarted && m_plan.phaseLocked && !m_locked && m_incoming->hasAudioPts())
        lock(outgoingPts, incomingPts);

    if (!m_promotionRequested && outgoingPts >= m_plan.handoffMs) {
        m_promotionRequested = true;
        Q_EMIT promotionRequested();
    }
    if (m_promoted && !m_outgoingStopped && outgoingPts >= m_plan.outgoingStopMs)
        stopOutgoing();

    returnSpeed(incomingPts);
    if (m_promoted && m_outgoingStopped && !m_returning)
        clear();
}

void TransitionRunner::stepShelves(double outgoingPts, double incomingPts)
{
    if (m_plan.outgoingBassCut.active())
        m_outgoing->setLowShelf(shelfGain(m_plan.outgoingBassCut, outgoingPts));
    if (m_plan.incomingBassEntry.active())
        m_incoming->setLowShelf(shelfGain(m_plan.incomingBassEntry, incomingPts));
}

void TransitionRunner::lock(double outgoingPts, double incomingPts)
{
    const double expected =
        m_plan.incomingStartMs + m_plan.incomingSpeed * (outgoingPts - m_plan.outgoingStartMs);
    const double sample = expected - incomingPts;
    m_lockError =
        m_lockErrorSeeded ? m_lockError + kLockSmoothing * (sample - m_lockError) : sample;
    m_lockErrorSeeded = true;
    const double error = m_lockError;
    m_lastLockError = error;
    if (std::abs(error) < kLockToleranceMs) {
        m_locked = true;
        m_incoming->setSpeed(m_plan.incomingSpeed);
        qCInfo(logTransition) << "lock_error_ms" << error;
        return;
    }

    const double correction =
        qBound(-kMaximumLockCorrection, double(error) * kLockGainPerSecond / 1000.0,
               kMaximumLockCorrection);
    m_incoming->setSpeed(m_plan.incomingSpeed * (1.0 + correction));
    if (outgoingPts >= m_plan.outgoingStartMs + kLockDeadlineMs)
        requestFallback();
}

void TransitionRunner::returnSpeed(double incomingPts)
{
    if (!m_returning && m_plan.phaseLocked && incomingPts >= m_plan.speedReturnMs) {
        m_returning = true;
        m_finishSpeed = m_plan.incomingSpeed;
        m_finishStartPts = incomingPts;
    }
    if (!m_returning)
        return;

    const qint64 length = m_finishing ? kFinishReturnMs : m_plan.speedReturnLengthMs;
    if (length <= 0) {
        m_incoming->setSpeed(1.0);
        m_returning = false;
        if (m_finishing)
            clear();
        return;
    }

    const double progress = m_finishing
        ? qBound(0.0, double(m_finishClock.elapsed()) / double(length), 1.0)
        : qBound(0.0, (incomingPts - m_finishStartPts) / double(length), 1.0);
    m_incoming->setSpeed(m_finishSpeed + (1.0 - m_finishSpeed) * progress);
    if (progress < 1.0)
        return;

    m_incoming->setSpeed(1.0);
    m_returning = false;
    if (m_finishing)
        clear();
}

void TransitionRunner::stopOutgoing()
{
    if (!active() || m_outgoingStopped)
        return;

    m_outgoing->stop();
    m_outgoing->forgetTransition();
    m_outgoingStopped = true;
}

void TransitionRunner::requestFallback()
{
    if (!active() || m_promotionRequested)
        return;

    qCInfo(logTransition) << "runner fallback" << "lock_error_ms" << m_lastLockError;
    abort();
    Q_EMIT fallbackRequested();
}

void TransitionRunner::clear()
{
    m_timer.stop();
    m_outgoing = nullptr;
    m_incoming = nullptr;
    m_plan = {};
    m_finishSpeed = 1.0;
    m_finishStartPts = 0.0;
    m_incomingStarted = false;
    m_locked = false;
    m_outgoingStopped = false;
    m_promoted = false;
    m_promotionRequested = false;
    m_returning = false;
    m_finishing = false;
    m_lastLockError = 0.0;
    m_lockError = 0.0;
    m_lockErrorSeeded = false;
}

}
