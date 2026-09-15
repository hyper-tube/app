#include "AudioEngine.h"

#include "PlaybackSettings.h"
#include "VideoOutput.h"
#include "core/Logging.h"

namespace {

constexpr qint64 kCrossfadeLeadMs = 1500;
constexpr qint64 kShortestCrossfadeMs = 1000;

}

namespace player {

AudioEngine::AudioEngine(QObject *parent)
    : QObject(parent)
    , m_runner(this)
{
    for (MpvController &deck : m_decks) {
        connect(&deck, &MpvController::positionChanged, this, [this, &deck](qint64 milliseconds) {
            if (&deck == &active())
                Q_EMIT positionChanged(milliseconds);
            if (m_retiring >= 0 && &deck == &m_decks.at(m_retiring) && m_outgoingStopMs
                && milliseconds >= *m_outgoingStopMs) {
                deck.stop();
                deck.setFade({});
                m_outgoingStopMs.reset();
                m_retiring = -1;
            }
        });
        connect(&deck, &MpvController::durationChanged, this, [this, &deck](qint64 milliseconds) {
            if (&deck == &active())
                Q_EMIT durationChanged(milliseconds);
        });
        connect(&deck, &MpvController::playingChanged, this, [this, &deck](bool playing) {
            if (&deck == &active())
                Q_EMIT playingChanged(playing);
        });
        connect(&deck, &MpvController::discontinuity, this, [this, &deck] {
            if (&deck == &active())
                Q_EMIT discontinuity();
        });
        connect(&deck, &MpvController::ending, this, [this, &deck] {
            if (&deck == &active())
                Q_EMIT ending();
        });
        connect(&deck, &MpvController::trackEnded, this, [this, &deck] {
            if (&deck == &active())
                Q_EMIT trackEnded();
        });
        connect(&deck, &MpvController::failed, this, [this, &deck](const QString &message) {
            if (m_runner.active() && &deck == &idle()) {
                m_runner.abort();
                Q_EMIT transitionFallbackRequested();
                return;
            }
            if (&deck == &active())
                Q_EMIT failed(message);
        });
        connect(&deck, &MpvController::filterChainFailed, this, [this] {
            if (!m_runner.active() || m_runner.promoted())
                return;
            m_runner.abort();
            Q_EMIT transitionFallbackRequested();
        });
        connect(&deck, &MpvController::loadedChanged, this, [this, &deck](bool loaded) {
            if (&deck != &idle())
                return;
            if (loaded)
                m_runner.incomingLoaded(deck);
            else
                cancelCrossfade();
            Q_EMIT stagedChanged();
        });
        connect(&deck, &MpvController::pictureSizeChanged, this, [this, &deck] {
            if (&deck == &active())
                Q_EMIT pictureChanged();
        });
    }

    connect(&VideoOutput::instance(), &VideoOutput::activeChanged, this,
            &AudioEngine::routePicture);
    connect(&VideoOutput::instance(), &VideoOutput::renderingChanged, this,
            &AudioEngine::routePicture);

    connect(&PlaybackSettings::instance(), &PlaybackSettings::equalizerChanged, this, [this] {
        for (MpvController &deck : m_decks)
            deck.refreshFilters();
    });

    connect(&m_runner, &TransitionRunner::promotionRequested, this,
            [this] { Q_EMIT transitionPromotionRequested(); });
    connect(&m_runner, &TransitionRunner::fallbackRequested, this,
            [this] { Q_EMIT transitionFallbackRequested(); });
}

bool AudioEngine::playing() const
{
    return active().playing();
}

bool AudioEngine::staged() const
{
    return !m_stagedVideoId.isEmpty() && idle().loaded();
}

bool AudioEngine::fading() const
{
    return m_runner.active() || (m_stagedVideoId.isEmpty() && idle().loaded());
}

bool AudioEngine::hasPicture() const
{
    return active().hasPicture();
}

int AudioEngine::pictureWidth() const
{
    return active().pictureWidth();
}

int AudioEngine::pictureHeight() const
{
    return active().pictureHeight();
}

MpvController &AudioEngine::active()
{
    return m_decks[m_active];
}

MpvController &AudioEngine::idle()
{
    return m_decks[1 - m_active];
}

void AudioEngine::load(const Stream &stream, qint64 startMilliseconds)
{
    m_runner.abort();
    discardStaged();
    idle().stop();
    idle().resetTransition();
    m_handoffMs.reset();
    m_outgoingStopMs.reset();
    m_retiring = -1;

    MpvController &deck = active();
    adopt(deck);
    deck.stop();
    deck.resetTransition();
    deck.setSpeed(m_rate);
    deck.setTrackGain(stream.gainDb);
    deck.load(stream.url, stream.userAgent, stream.cookie, startMilliseconds, false);
    deck.setPictureUrl(stream.pictureUrl);
    routePicture();
    Q_EMIT pictureChanged();
}

void AudioEngine::stage(const Stream &stream)
{
    if (m_stagedVideoId == stream.videoId || fading())
        return;

    cancelCrossfade();
    MpvController &deck = idle();
    deck.resetTransition();
    deck.setLoopFile(false);
    deck.setVolume(m_volume);
    deck.setMuted(m_muted);
    deck.setFade({});
    deck.setTrackGain(stream.gainDb);
    deck.load(stream.url, stream.userAgent, stream.cookie, 0, true);
    deck.setPictureUrl(stream.pictureUrl);

    m_stagedVideoId = stream.videoId;
    qCDebug(logPlayback) << "staged" << m_stagedVideoId;
    Q_EMIT stagedChanged();
}

bool AudioEngine::stage(const Stream &stream, const TransitionPlan &plan)
{
    if (m_stagedVideoId == stream.videoId || fading()
        || plan.outgoingStartMs < active().position() + kCrossfadeLeadMs) {
        qCInfo(logTransition) << "runner rejected" << stream.videoId << "staged" << m_stagedVideoId
                              << "fading" << fading() << "outgoing_ms" << plan.outgoingStartMs
                              << "position_ms" << active().position();
        return false;
    }

    cancelCrossfade();
    MpvController &deck = idle();
    deck.stop();
    deck.resetTransition();
    deck.setLoopFile(false);
    deck.setVolume(m_volume);
    deck.setMuted(m_muted);
    deck.setTrackGain(stream.gainDb);
    deck.setPictureUrl(stream.pictureUrl);
    if (!m_runner.prepare(active(), deck, plan)) {
        qCInfo(logTransition) << "runner unavailable" << stream.videoId;
        deck.resetTransition();
        return false;
    }

    deck.load(stream.url, stream.userAgent, stream.cookie, plan.incomingStartMs, true);
    m_stagedVideoId = stream.videoId;
    qCInfo(logTransition) << "runner staged" << m_stagedVideoId << qUtf8Printable(plan.kindName());
    Q_EMIT stagedChanged();
    return true;
}

void AudioEngine::fallbackToPhaseTwo(const Stream &stream)
{
    if (m_stagedVideoId != stream.videoId)
        return;

    m_runner.abort();
    idle().stop();
    idle().resetTransition();
    m_stagedVideoId.clear();
    stage(stream);
}

void AudioEngine::discardStaged()
{
    if (m_stagedVideoId.isEmpty() && !m_runner.active())
        return;

    m_runner.abort();
    cancelCrossfade();
    m_stagedVideoId.clear();
    idle().stop();
    idle().resetTransition();
    Q_EMIT stagedChanged();
}

void AudioEngine::endTransition()
{
    if (m_stagedVideoId.isEmpty() && m_runner.active()) {
        m_runner.finish(false);
        return;
    }
    discardStaged();
}

std::optional<qint64> AudioEngine::scheduleCrossfade(qint64 fadeMilliseconds,
                                                     qint64 audibleEndMilliseconds,
                                                     qint64 incomingStartMilliseconds)
{
    if (m_handoffMs || !staged())
        return m_handoffMs;

    MpvController &outgoing = active();
    MpvController &incoming = idle();
    const qint64 duration = outgoing.duration();
    const qint64 end =
        audibleEndMilliseconds > 0 ? qBound(qint64(0), audibleEndMilliseconds, duration) : duration;
    const qint64 start = qMax(end - fadeMilliseconds, outgoing.position() + kCrossfadeLeadMs);
    const qint64 length = end - start;
    if (length < kShortestCrossfadeMs)
        return std::nullopt;

    const qint64 entry = qMax(qint64(0), incomingStartMilliseconds);
    outgoing.setFade({Fade::Out, start, length});
    incoming.setFade({Fade::In, entry, length});
    incoming.seek(entry);
    m_handoffMs = start;
    if (end < duration)
        m_outgoingStopMs = end;
    qCInfo(logTransition) << "plan fixed" << "handoff_ms" << start << "length_ms" << length
                          << "stop_ms" << end << "incoming_start_ms" << entry << "trimmed"
                          << (end < duration || entry > 0);
    return m_handoffMs;
}

std::optional<qint64> AudioEngine::scheduleSmartTransition(qint64 fadeStartMilliseconds,
                                                           qint64 stopMilliseconds,
                                                           qint64 incomingStartMilliseconds,
                                                           bool naturalFade)
{
    if (m_handoffMs || !staged())
        return m_handoffMs;

    MpvController &outgoing = active();
    MpvController &incoming = idle();
    const qint64 start = qBound(qint64(0), fadeStartMilliseconds, outgoing.duration());
    const qint64 stop = qBound(start, stopMilliseconds, outgoing.duration());
    const qint64 length = stop - start;
    if (start < outgoing.position() + kCrossfadeLeadMs || length < kShortestCrossfadeMs)
        return std::nullopt;

    if (naturalFade)
        outgoing.setFade({});
    else
        outgoing.setFade({Fade::Out, start, length});
    incoming.setFade({Fade::In, incomingStartMilliseconds, length});
    incoming.seek(incomingStartMilliseconds);
    m_handoffMs = start;
    m_outgoingStopMs = stop;
    qCInfo(logTransition) << "plan smart" << "handoff_ms" << start << "stop_ms" << stop
                          << "incoming_start_ms" << incomingStartMilliseconds << "length_ms"
                          << length << "natural_fade" << naturalFade;
    return m_handoffMs;
}

void AudioEngine::promoteStaged(bool crossfade)
{
    if (m_stagedVideoId.isEmpty())
        return;

    const bool runner = m_runner.active();
    if (runner && !crossfade)
        m_runner.finish(false);
    const bool fade = !runner && crossfade && m_handoffMs.has_value();
    MpvController &outgoing = active();
    MpvController &incoming = idle();
    if (!fade && !runner && incoming.fade().active()) {
        incoming.setFade({});
        incoming.seek(incoming.position());
    }

    m_active = 1 - m_active;
    m_stagedVideoId.clear();
    m_handoffMs.reset();
    adopt(incoming);

    if (!fade && !runner) {
        outgoing.stop();
        outgoing.setFade({});
        m_outgoingStopMs.reset();
        m_retiring = -1;
    } else if (!runner && m_outgoingStopMs) {
        m_retiring = 1 - m_active;
    }
    incoming.play();
    if (runner)
        m_runner.confirmPromotion();

    routePicture();
    Q_EMIT pictureChanged();
    Q_EMIT stagedChanged();
    Q_EMIT durationChanged(incoming.duration());
    Q_EMIT positionChanged(incoming.position());
}

void AudioEngine::play()
{
    active().play();
}

void AudioEngine::pause()
{
    endFade();
    active().pause();
}

void AudioEngine::stop()
{
    m_runner.abort();
    discardStaged();
    m_handoffMs.reset();
    m_outgoingStopMs.reset();
    m_retiring = -1;
    for (MpvController &deck : m_decks) {
        deck.stop();
        deck.resetTransition();
    }
    Q_EMIT pictureChanged();
}

void AudioEngine::setRate(double rate)
{
    if (qFuzzyCompare(m_rate, rate))
        return;
    m_rate = rate;
    if (!m_runner.active() && !staged())
        active().setSpeed(rate);
}

void AudioEngine::seek(qint64 milliseconds)
{
    endFade();
    active().seek(milliseconds);
}

void AudioEngine::setLoopFile(bool loop)
{
    m_loop = loop;
    active().setLoopFile(loop);
}

void AudioEngine::setVolume(int volume)
{
    m_volume = qBound(0, volume, 100);
    for (MpvController &deck : m_decks)
        deck.setVolume(m_volume);
}

void AudioEngine::setMuted(bool muted)
{
    m_muted = muted;
    for (MpvController &deck : m_decks)
        deck.setMuted(m_muted);
}

void AudioEngine::setPicture(const QUrl &url)
{
    active().setPictureUrl(url);
    routePicture();
    Q_EMIT pictureChanged();
}

void AudioEngine::routePicture()
{
    VideoOutput &output = VideoOutput::instance();
    MpvController &deck = active();
    const bool wanted = output.active() && deck.hasPicture();

    idle().setPictureEnabled(false);
    deck.setPictureEnabled(wanted && output.rendering() == deck.handle());
    output.setSource(wanted ? deck.handle() : nullptr);
}

void AudioEngine::adopt(MpvController &deck) const
{
    deck.setVolume(m_volume);
    deck.setMuted(m_muted);
    deck.setLoopFile(m_loop);
}

void AudioEngine::cancelCrossfade()
{
    if (!m_handoffMs)
        return;

    m_handoffMs.reset();
    m_outgoingStopMs.reset();
    m_retiring = -1;
    active().setFade({});
}

void AudioEngine::endFade()
{
    if (m_runner.active()) {
        if (m_runner.promoted())
            m_runner.finish(true);
        return;
    }
    if (!fading())
        return;

    idle().stop();
    active().setFade({});
    m_outgoingStopMs.reset();
    m_retiring = -1;
}

}
