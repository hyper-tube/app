#include "PlaybackController.h"

#include "core/Logging.h"
#include "diagnostics/Diagnostics.h"
#include "library/Downloads.h"
#include "library/LibraryActions.h"
#include "library/PlayLog.h"
#include "net/Connectivity.h"
#include "player/PlaybackSettings.h"
#include "player/TransitionPlanner.h"
#include "player/VideoOutput.h"

#include <QCoreApplication>
#include <QMetaEnum>
#include <QPointer>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSet>
#include <QUrl>
#include <QUrlQuery>

#include <algorithm>

namespace {

constexpr qint64 kRestartThresholdMs = 3000;
constexpr qint64 kResumeTailMs = 15000;
constexpr qint64 kStageLeadMs = 15000;
constexpr qint64 kShortestSmartTransitionMs = 1000;
constexpr int kAutoplayLead = 3;

const QString kRadioPrefix = QStringLiteral("RDAMVM");
const QRegularExpression kCodecs(QStringLiteral("codecs=\"?([^\",]+)"));

struct SmartTransition
{
    qint64 fadeStartMs = 0;
    qint64 stopMs = 0;
    qint64 incomingStartMs = 0;
    bool naturalFade = false;
};

const char *kindOf(const media::Track &track)
{
    if (track.live)
        return "live";
    if (track.episode)
        return "episode";
    if (track.upload)
        return "upload";
    return track.video ? "video" : "song";
}

diagnostics::Value codecOf(const QString &mimeType)
{
    return diagnostics::Value::symbol(kCodecs.match(mimeType).captured(1));
}

const char *transitionModeName(player::PlaybackSettings::TransitionMode mode)
{
    return QMetaEnum::fromType<player::PlaybackSettings::TransitionMode>().valueToKey(mode);
}

QString videoIdFrom(const QString &token)
{
    if (!token.contains(QLatin1Char('/')) && !token.contains(QLatin1Char('=')))
        return token;

    const QUrl url(token);
    const QString parameter = QUrlQuery(url).queryItemValue(QStringLiteral("v"));
    if (!parameter.isEmpty())
        return parameter;

    return url.path().section(QLatin1Char('/'), -1);
}

QList<media::Track> tracksFrom(const QString &input)
{
    static const QRegularExpression separator(QStringLiteral("[\\s,]+"));

    QList<media::Track> tracks;
    for (const QString &token : input.split(separator, Qt::SkipEmptyParts)) {
        const QString videoId = videoIdFrom(token);
        if (videoId.isEmpty())
            continue;

        media::Track track;
        track.videoId = videoId;
        track.artId = QStringLiteral("https://i.ytimg.com/vi/%1/hqdefault.jpg").arg(videoId);
        tracks.append(track);
    }
    return tracks;
}

int crossfadeMilliseconds()
{
    return player::PlaybackSettings::instance().crossfadeSeconds() * 1000;
}

std::optional<analysis::Analyzer::Depth> analysisDepth()
{
    switch (player::PlaybackSettings::instance().transitionMode()) {
    case player::PlaybackSettings::TransitionsOff: return std::nullopt;
    case player::PlaybackSettings::Crossfade: return analysis::Analyzer::Depth::Loudness;
    case player::PlaybackSettings::Smart: return analysis::Analyzer::Depth::Full;
    }
    return std::nullopt;
}

std::optional<SmartTransition> smartTransitionFor(const analysis::TrackAnalysis &outgoing,
                                                  const analysis::TrackAnalysis &incoming,
                                                  qint64 maximumMs)
{
    if (maximumMs < kShortestSmartTransitionMs)
        return std::nullopt;

    const qint64 stopMs = qBound(qint64(0), outgoing.trailingSilenceStartMs, outgoing.durationMs);
    if (stopMs < kShortestSmartTransitionMs)
        return std::nullopt;

    const bool hasNaturalFade = outgoing.fadeOutStartMs >= 0
        && outgoing.fadeOutStartMs <= stopMs - kShortestSmartTransitionMs;
    const qint64 fadeStartMs = hasNaturalFade ? qMax(outgoing.fadeOutStartMs, stopMs - maximumMs)
                                              : qMax(qint64(0), stopMs - maximumMs);
    if (stopMs - fadeStartMs < kShortestSmartTransitionMs)
        return std::nullopt;

    SmartTransition transition;
    transition.fadeStartMs = fadeStartMs;
    transition.stopMs = stopMs;
    transition.incomingStartMs =
        qBound(qint64(0), incoming.leadingSilenceEndMs, incoming.durationMs);
    transition.naturalFade = hasNaturalFade;
    return transition;
}

}

namespace media {

PlaybackController::PlaybackController(QObject *parent)
    : QObject(parent)
    , m_analyzer(this)
    , m_resolver(innertube::Session::instance())
    , m_prefetch(innertube::Session::instance())
    , m_picture(innertube::Session::instance())
    , m_analysisCurrent(innertube::Session::instance())
    , m_analysisNext(innertube::Session::instance())
    , m_tracker(this)
    , m_engine(this)
    , m_radio(innertube::Session::instance(), this)
{
    connect(&m_tracker, &player::PlaybackTracker::pingRequested, this,
            [](const QString &key, const QUrl &url) {
        auto &session = innertube::Session::instance();
        const auto *client = session.clients().client(key);
        if (client && session.authenticated())
            session.ping(*client, url);
    });
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, [this] {
        finishTracking();
        m_loadedVideoId.clear();
        setPending(false);
        m_engine.pause();
    });
    connect(&innertube::Session::instance(), &innertube::Session::identityChanged, this, [this] {
        ++m_trackingGeneration;
        m_tracker.cancel();
        m_activeStream.tracking = {};
        dropStaging();
    });
    connect(&innertube::Session::instance(), &innertube::Session::authenticatedChanged, this,
            [this] {
        ++m_trackingGeneration;
        m_tracker.cancel();
        m_activeStream.tracking = {};
        dropStaging();
    });
    connect(&m_engine, &player::AudioEngine::ending, this, &PlaybackController::finishTracking);
    connect(&m_engine, &player::AudioEngine::discontinuity, &m_tracker,
            &player::PlaybackTracker::discontinuity);
    connect(&library::LibraryActions::instance(), &library::LibraryActions::trackRatingChanged,
            this, [this](const QString &videoId, int rating) {
        bool touched = false;
        for (Track &entry : m_queue) {
            if (entry.videoId != videoId)
                continue;
            entry.liked = rating > 0;
            entry.disliked = rating < 0;
            entry.ratingKnown = true;
            touched = true;
        }
        if (!touched)
            return;
        Q_EMIT queueChanged();
        if (track().videoId == videoId)
            Q_EMIT trackChanged();
    });

    connect(&library::LibraryActions::instance(), &library::LibraryActions::trackEntryRead, this,
            [this](const model::Item &entry) {
        if (entry.track.videoId != track().videoId)
            return;
        m_currentEntry = entry;
        const bool resolved =
            m_currentStream.videoId == entry.track.videoId && m_currentStream.episode.has_value();
        if (!entry.track.episode || m_queue.at(m_queueIndex).episode || resolved)
            return;
        m_queue[m_queueIndex].episode = true;
        m_queue[m_queueIndex].video = true;
        applyRate();
        Q_EMIT queueChanged();
        Q_EMIT trackChanged();
    });
    connect(&library::LibraryActions::instance(), &library::LibraryActions::trackDisliked, this,
            [this](const QString &videoId) {
        if (videoId != track().videoId)
            return;
        library::PlayLog::instance().finish();
        advanceQueue();
    });

    connect(&m_resolver, &player::StreamResolver::resolved, this, &PlaybackController::applyStream);
    connect(&m_resolver, &player::StreamResolver::failed, this, &PlaybackController::reportFailure);

    connect(&m_analysisCurrent, &player::StreamResolver::resolved, this,
            [this](const player::Stream &stream) { requestAnalysisFor(stream); });
    connect(&m_analysisNext, &player::StreamResolver::resolved, this,
            [this](const player::Stream &stream) { requestAnalysisFor(stream); });
    connect(&m_analysisCurrent, &player::StreamResolver::failed, this,
            [](const QString &videoId, const QString &message) {
        qCDebug(logTransition) << "cannot resolve current analysis" << videoId << message;
    });
    connect(&m_analysisNext, &player::StreamResolver::failed, this,
            [](const QString &videoId, const QString &message) {
        qCDebug(logTransition) << "cannot resolve next analysis" << videoId << message;
    });
    connect(&m_analyzer, &analysis::Analyzer::ready, this,
            [this](const analysis::TrackAnalysis &) { considerStaging(); });

    connect(&m_prefetch, &player::StreamResolver::resolved, this,
            [this](const player::Stream &stream) {
        if (stream.videoId != m_stagingVideoId)
            return;
        m_stagedStream = asPlayed(stream);
        requestAnalysisFor(m_stagedStream);
        const std::optional<player::TransitionPlan> plan = m_pendingRunnerPlan;
        m_pendingRunnerPlan.reset();
        m_runnerStaged = plan && m_engine.stage(m_stagedStream, *plan);
        if (!m_runnerStaged)
            m_engine.stage(m_stagedStream);
    });
    connect(&m_prefetch, &player::StreamResolver::failed, this,
            [this](const QString &videoId, const QString &message) {
        if (videoId != m_stagingVideoId)
            return;
        qCDebug(logPlayback) << "cannot stage" << videoId << message;
        dropStaging();
    });

    connect(&m_picture, &player::StreamResolver::resolved, this,
            [this](const player::Stream &stream) {
        if (stream.videoId != m_loadedVideoId)
            return;
        if (asPlayed(stream).showsPicture()) {
            m_engine.setPicture(stream.pictureUrl);
            setStreamVideo(true);
        }
        setPictureResolving(false);
    });
    connect(&m_picture, &player::StreamResolver::failed, this,
            [this](const QString &videoId, const QString &message) {
        qCDebug(logPlayback) << "no picture for" << videoId << message;
        if (videoId == m_loadedVideoId)
            setPictureResolving(false);
    });

    connect(&m_radio, &Radio::extended, this, &PlaybackController::appendTracks);

    connect(&m_engine, &player::AudioEngine::positionChanged, this, [this](qint64 milliseconds) {
        if (m_loadedVideoId.isEmpty())
            return;
        if (m_repeat == RepeatOne && !episode() && m_duration > 0 && m_position > m_duration - 2000
            && milliseconds < 2000) {
            library::PlayLog::instance().begin(track());
            player::Stream replay = m_activeStream;
            replay.tracking = {};
            prepareTracking(replay);
            m_tracker.setPlaying(m_engine.playing());
        }
        m_tracker.observe(milliseconds);
        setPosition(milliseconds);
        if (live())
            return;
        library::PlayLog::instance().reach(milliseconds, m_duration);
        considerStaging();
    });
    connect(&m_engine, &player::AudioEngine::durationChanged, this, [this](qint64 milliseconds) {
        if (!m_loadedVideoId.isEmpty() && milliseconds > 0 && !live())
            setDuration(milliseconds);
    });
    connect(&m_engine, &player::AudioEngine::playingChanged, this, [this](bool playing) {
        m_tracker.setPlaying(playing && !m_loadedVideoId.isEmpty());
        if (playing && !m_loadedVideoId.isEmpty())
            setPending(false);
        updatePlaying();
    });
    connect(&m_engine, &player::AudioEngine::trackEnded, this, &PlaybackController::handleTrackEnd);
    connect(&m_engine, &player::AudioEngine::failed, this, [this](const QString &message) {
        finishTracking();
        qCWarning(logPlayback) << "playback failed" << track().videoId << message;
        diagnostics::breadcrumb(
            "playback.engine_failed",
            {{"source", m_loadedLocal ? "download" : "stream"}, {"kind", kindOf(track())}},
            diagnostics::Level::Error);
        const bool streamed = !m_loadedLocal;
        m_loadedVideoId.clear();
        setPending(false);
        if (streamed)
            net::Connectivity::instance().check();
        if (streamed && !net::Connectivity::instance().online()) {
            reportOffline();
            return;
        }
        setError(track().upload ? tr("YouTube would not stream this upload.")
                                : tr("Playback failed. Press play to retry."));
    });
    connect(&m_engine, &player::AudioEngine::transitionPromotionRequested, this, [this] {
        if (m_runnerStaged && stagedIndex() == m_queueIndex + 1)
            promoteStaged(true);
    });
    connect(&m_engine, &player::AudioEngine::transitionFallbackRequested, this, [this] {
        if (!m_stagedStream.valid())
            return;
        diagnostics::breadcrumb("playback.transition_fallback", {{"runner", m_runnerStaged}},
                                diagnostics::Level::Warning);
        m_runnerStaged = false;
        m_pendingRunnerPlan.reset();
        m_engine.fallbackToPhaseTwo(m_stagedStream);
    });

    connect(&player::PlaybackSettings::instance(), &player::PlaybackSettings::crossfadeChanged,
            this, [this] { dropTransition(); });
    connect(&player::PlaybackSettings::instance(), &player::PlaybackSettings::transitionModeChanged,
            this, [this] {
        dropTransition();
        requestAnalyses();
    });
    connect(&player::PlaybackSettings::instance(), &player::PlaybackSettings::matchTempoChanged,
            this, [this] { dropTransition(); });

    connect(&m_engine, &player::AudioEngine::pictureChanged, this,
            &PlaybackController::videoPictureChanged);
    connect(&player::VideoOutput::instance(), &player::VideoOutput::supportedChanged, this,
            &PlaybackController::videoAvailableChanged);
    connect(&player::PlaybackSettings::instance(), &player::PlaybackSettings::playVideosChanged,
            this, [this] { setVideoShown(player::PlaybackSettings::instance().playVideos()); });
    connect(&player::PlaybackSettings::instance(), &player::PlaybackSettings::podcastSpeedChanged,
            this, &PlaybackController::applyRate);

    connect(&net::Connectivity::instance(), &net::Connectivity::onlineChanged, this,
            &PlaybackController::followConnectivity);

    m_videoShown = player::PlaybackSettings::instance().playVideos();
    player::VideoOutput::instance().setWanted(m_videoShown);
}

PlaybackController &PlaybackController::instance()
{
    static auto *controller = new PlaybackController(QCoreApplication::instance());
    return *controller;
}

PlaybackController *PlaybackController::create(QQmlEngine *, QJSEngine *)
{
    QQmlEngine::setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
    return &instance();
}

Track PlaybackController::track() const
{
    if (m_queueIndex < 0 || m_queueIndex >= m_queue.size())
        return {};
    return m_queue.at(m_queueIndex);
}

int PlaybackController::queueOffset() const
{
    return qBound(0, m_queueIndex - 50, qMax(0, int(m_queue.size()) - 100));
}

QList<Track> PlaybackController::queueWindow() const
{
    return m_queue.mid(queueOffset(), 100);
}

bool PlaybackController::canGoNext() const
{
    if (m_queue.isEmpty())
        return false;
    if (repeating())
        return true;
    return m_queueIndex < m_queue.size() - 1;
}

bool PlaybackController::repeating() const
{
    return m_repeat != RepeatOff && !episode();
}

bool PlaybackController::canGoPrevious() const
{
    return !m_queue.isEmpty();
}

void PlaybackController::setShuffle(bool shuffle)
{
    if (m_shuffle == shuffle)
        return;

    m_shuffle = shuffle;
    if (shuffle)
        shuffleFrom(m_queueIndex);
    else
        restoreOrder();
    Q_EMIT shuffleChanged();
}

void PlaybackController::setRepeat(RepeatMode repeat)
{
    if (m_repeat == repeat)
        return;
    m_repeat = repeat;
    m_engine.setLoopFile(repeat == RepeatOne && !episode());
    if (repeat == RepeatOne)
        dropStaging();
    Q_EMIT repeatChanged();
    Q_EMIT reachChanged();
}

void PlaybackController::setVolume(int volume)
{
    const int bounded = qBound(0, volume, 100);
    if (m_volume == bounded)
        return;
    m_volume = bounded;
    m_engine.setVolume(bounded);
    Q_EMIT volumeChanged();
}

void PlaybackController::setMuted(bool muted)
{
    if (m_muted == muted)
        return;
    m_muted = muted;
    m_engine.setMuted(muted);
    Q_EMIT mutedChanged();
}

void PlaybackController::resumeAt(qint64 milliseconds)
{
    if (milliseconds <= 0 || !track().valid() || live())
        return;
    m_resumePosition = milliseconds;
    setPosition(milliseconds);
}

void PlaybackController::setQueue(const QList<media::Track> &tracks, int index,
                                  const model::Item &source)
{
    adoptQueue(tracks, index, source);
    if (m_shuffle && m_queue.size() > 1 && !episode()) {
        m_queue.move(m_queueIndex, 0);
        m_queueIndex = 0;
        shuffleFrom(0);
        Q_EMIT queueIndexChanged();
        Q_EMIT trackChanged();
    }
}

void PlaybackController::restoreQueue(const QList<media::Track> &tracks, int index,
                                      const model::Item &source)
{
    adoptQueue(tracks, index, source);
}

void PlaybackController::continueRemoteQueue(const QList<media::Track> &tracks, int index,
                                             const model::Item &source)
{
    adoptQueue(tracks, index, source);
    Q_EMIT remoteQueueContinued();
}

void PlaybackController::clearQueue()
{
    adoptQueue({}, -1, {});
}

void PlaybackController::setQueueSource(const model::Item &source)
{
    if (m_queueSource.title == source.title && m_queueSource.kind == source.kind
        && m_queueSource.browseId == source.browseId
        && m_queueSource.playlistId == source.playlistId && m_queueSource.artId == source.artId
        && m_queueSource.subtitle == source.subtitle)
        return;
    m_queueSource = source;
    Q_EMIT queueSourceChanged();
}

void PlaybackController::adoptQueue(const QList<Track> &tracks, int index,
                                    const model::Item &source)
{
    finishTracking();
    setError({});
    dropStaging();
    m_radio.reset();
    m_queue = tracks;
    m_unshuffled = tracks;
    setQueueSource(tracks.isEmpty() ? model::Item() : source);
    if (tracks.isEmpty())
        m_engine.stop();
    else
        m_engine.pause();
    m_loadedVideoId.clear();
    setPending(false);
    selectTrack(tracks.isEmpty() ? -1 : qBound(0, index, int(tracks.size()) - 1), 1);
    Q_EMIT queueChanged();
    Q_EMIT reachChanged();
}

void PlaybackController::shuffleFrom(int keep)
{
    const int first = qMax(0, keep + 1);
    if (m_queue.size() - first < 2) {
        Q_EMIT queueChanged();
        return;
    }

    dropStaging();
    for (int position = m_queue.size() - 1; position > first; --position) {
        const int target = first + QRandomGenerator::global()->bounded(position - first + 1);
        m_queue.swapItemsAt(position, target);
    }
    Q_EMIT queueChanged();
    requestAnalyses();
}

void PlaybackController::restoreOrder()
{
    if (m_unshuffled.size() != m_queue.size()) {
        Q_EMIT queueChanged();
        return;
    }

    dropStaging();
    const QString current = track().videoId;
    m_queue = m_unshuffled;

    const int restored = indexOf(current);
    if (restored >= 0)
        m_queueIndex = restored;

    Q_EMIT queueChanged();
    Q_EMIT queueIndexChanged();
    Q_EMIT reachChanged();
    requestAnalyses();
}

QList<media::Track> PlaybackController::unknownTracks(const QList<Track> &tracks) const
{
    QSet<QString> known;
    known.reserve(m_queue.size());
    for (const Track &entry : m_queue)
        known.insert(entry.videoId);

    QList<Track> fresh;
    for (const Track &entry : tracks) {
        if (!entry.valid() || known.contains(entry.videoId))
            continue;
        known.insert(entry.videoId);
        fresh.append(entry);
    }
    return fresh;
}

void PlaybackController::appendTracks(const QList<Track> &tracks)
{
    const QList<Track> fresh = unknownTracks(tracks);
    if (fresh.isEmpty())
        return;

    const int previousOffset = queueOffset();
    m_queue.append(fresh);
    m_unshuffled.append(fresh);
    qCDebug(logPlayback) << "queued" << fresh.size() << "tracks";

    if (queueOffset() != previousOffset)
        Q_EMIT queueIndexChanged();
    Q_EMIT queueChanged();
    Q_EMIT reachChanged();
    requestAnalyses();
}

void PlaybackController::playNext(const QList<media::Track> &tracks)
{
    insertTracks(tracks, true);
}

void PlaybackController::enqueue(const QList<media::Track> &tracks)
{
    insertTracks(tracks, false);
}

model::Item PlaybackController::queueEntry(int index) const
{
    model::Item entry;
    if (index < 0 || index >= m_queue.size())
        return entry;
    entry.track = m_queue.at(index);
    entry.title = entry.track.title;
    entry.subtitle = entry.track.artist;
    entry.artId = entry.track.artId;
    entry.kind = entry.track.episode ? QStringLiteral("episode") : QStringLiteral("song");
    return entry;
}

model::Item PlaybackController::currentEntry() const
{
    model::Item entry = queueEntry(m_queueIndex);
    if (!entry.playable())
        return entry;
    if (m_currentEntry.track.videoId == entry.track.videoId) {
        entry.actions = m_currentEntry.actions;
        entry.actions.setVideoId.clear();
        entry.actions.sourcePlaylistId.clear();
        entry.pinned = m_currentEntry.pinned;
    }
    if (entry.track.episode && entry.actions.podcastId.isEmpty()
        && m_queueSource.kind == QLatin1String("podcast"))
        entry.actions.podcastId = m_queueSource.browseId;
    if (!entry.actions.mixable() && !entry.track.live && !entry.track.episode) {
        entry.actions.mixPlaylistId = kRadioPrefix + entry.track.videoId;
        entry.actions.mixVideoId = entry.track.videoId;
    }
    library::LibraryActions::instance().applyOverrides(entry);
    return entry;
}

void PlaybackController::repeatCurrent(bool next)
{
    const Track current = track();
    if (!current.valid())
        return;

    const int previousOffset = queueOffset();
    if (next) {
        dropStaging();
        m_queue.insert(m_queueIndex + 1, current);
        const int mirrored =
            qBound(0, int(m_unshuffled.indexOf(current)) + 1, int(m_unshuffled.size()));
        m_unshuffled.insert(mirrored, current);
    } else {
        m_queue.append(current);
        m_unshuffled.append(current);
    }
    if (queueOffset() != previousOffset)
        Q_EMIT queueIndexChanged();
    Q_EMIT queueChanged();
    Q_EMIT reachChanged();
    requestAnalyses();
}

void PlaybackController::removeFromQueue(int index)
{
    if (index < 0 || index >= m_queue.size())
        return;
    if (index == m_queueIndex) {
        removeCurrent();
        return;
    }
    dropStaging();
    m_unshuffled.removeOne(m_queue.at(index));
    m_queue.removeAt(index);
    if (index < m_queueIndex)
        --m_queueIndex;
    Q_EMIT queueChanged();
    Q_EMIT queueIndexChanged();
    Q_EMIT reachChanged();
    requestAnalyses();
}

void PlaybackController::removeCurrent()
{
    if (m_queue.size() <= 1) {
        clearQueue();
        return;
    }

    const bool resume = m_playing;
    const bool last = m_queueIndex == m_queue.size() - 1;
    dropStaging();
    m_engine.pause();
    m_loadedVideoId.clear();
    setPending(false);
    m_unshuffled.removeOne(m_queue.at(m_queueIndex));
    m_queue.removeAt(m_queueIndex);
    const int target = last ? int(m_queue.size()) - 1 : m_queueIndex;
    m_queueIndex = -1;
    selectTrack(target, last ? -1 : 1);
    Q_EMIT queueChanged();
    if (resume && !last)
        playAt(target);
}

void PlaybackController::insertTracks(const QList<Track> &tracks, bool next)
{
    if (m_queue.isEmpty()) {
        setQueue(tracks, 0);
        playAt(0);
        return;
    }
    const QList<Track> fresh = unknownTracks(tracks);
    if (fresh.isEmpty())
        return;
    if (!next) {
        appendTracks(fresh);
        return;
    }

    dropStaging();
    const int after = qBound(0, m_queueIndex + 1, int(m_queue.size()));
    for (int i = 0; i < fresh.size(); ++i)
        m_queue.insert(after + i, fresh.at(i));
    const int mirrored = qBound(0, m_unshuffled.indexOf(track()) + 1, int(m_unshuffled.size()));
    for (int i = 0; i < fresh.size(); ++i)
        m_unshuffled.insert(mirrored + i, fresh.at(i));
    Q_EMIT queueChanged();
    Q_EMIT queueIndexChanged();
    Q_EMIT reachChanged();
    requestAnalyses();
}

int PlaybackController::indexOf(const QString &videoId) const
{
    for (int position = 0; position < m_queue.size(); ++position) {
        if (m_queue.at(position).videoId == videoId)
            return position;
    }
    return -1;
}

void PlaybackController::playVideoIds(const QString &input)
{
    const QList<Track> tracks = tracksFrom(input);
    if (tracks.isEmpty())
        return;

    setQueue(tracks, 0);
    playAt(0);
}

void PlaybackController::playQueue(const QList<media::Track> &tracks, int index,
                                   const model::Item &source)
{
    if (index < 0 || index >= tracks.size())
        return;
    if (!reachable(tracks.at(index))) {
        reportOffline();
        return;
    }
    setQueue(tracks, index, source);
    playAt(m_queueIndex);
}

void PlaybackController::playAt(int index)
{
    if (index < 0 || index >= m_queue.size())
        return;
    if (!reachable(m_queue.at(index))) {
        reportOffline();
        return;
    }

    if (index != m_queueIndex)
        selectTrack(index, index > m_queueIndex ? 1 : -1);
    play();
}

void PlaybackController::play()
{
    if (!track().valid())
        return;

    if (m_loadedVideoId == track().videoId) {
        m_engine.play();
        return;
    }
    startCurrent();
}

void PlaybackController::pause()
{
    setPending(false);
    if (m_engine.transitionStarted() && !m_engine.transitionPromoted())
        dropStaging();
    m_engine.pause();
}

void PlaybackController::toggle()
{
    if (m_playing)
        pause();
    else
        play();
}

void PlaybackController::next()
{
    diagnostics::breadcrumb("playback.next_requested",
                            {{"queue_index", m_queueIndex},
                             {"queue_size", m_queue.size()},
                             {"staged", stagedIndex() == m_queueIndex + 1}});
    library::PlayLog::instance().reach(m_duration, m_duration);
    library::PlayLog::instance().finish();
    advanceQueue();
}

void PlaybackController::advanceQueue()
{
    if (stagedIndex() == m_queueIndex + 1) {
        promoteStaged(false);
        return;
    }
    if (advance(1))
        startCurrent();
    else
        stopAtEnd();
}

void PlaybackController::previous()
{
    diagnostics::breadcrumb("playback.previous_requested",
                            {{"queue_index", m_queueIndex}, {"queue_size", m_queue.size()}});
    if (!m_loadedVideoId.isEmpty() && m_position > kRestartThresholdMs && !live()) {
        seek(0);
        return;
    }
    if (advance(-1))
        startCurrent();
    else
        seek(0);
}

void PlaybackController::seek(qint64 milliseconds)
{
    if (m_loadedVideoId.isEmpty() || live())
        return;

    if (m_engine.transitionRunning() && !m_engine.transitionPromoted())
        dropStaging();
    const qint64 target = qBound(qint64(0), milliseconds, m_duration);
    m_engine.seek(target);
    setPosition(target);
    Q_EMIT seeked(target);
}

void PlaybackController::skip(qint64 milliseconds)
{
    if (m_loadedVideoId.isEmpty() || live())
        return;
    seek(m_position + milliseconds);
}

void PlaybackController::cycleRepeat()
{
    setRepeat(static_cast<RepeatMode>((m_repeat + 1) % 3));
}

void PlaybackController::toggleMuted()
{
    setMuted(!m_muted);
}

void PlaybackController::toggleLike() const
{
    const Track current = track();
    if (current.valid())
        library::LibraryActions::instance().setTrackLiked(current.videoId, !current.liked);
}

QString PlaybackController::formatTime(qint64 milliseconds)
{
    milliseconds = std::max<qint64>(milliseconds, 0);

    const qint64 total = milliseconds / 1000;
    const qint64 hours = total / 3600;
    const qint64 minutes = (total % 3600) / 60;
    const qint64 seconds = total % 60;

    if (hours > 0) {
        return QStringLiteral("%1:%2:%3")
            .arg(hours)
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'));
    }
    return QStringLiteral("%1:%2").arg(minutes).arg(seconds, 2, 10, QLatin1Char('0'));
}

bool PlaybackController::videoAvailable() const
{
    if (!player::VideoOutput::instance().supported())
        return false;
    const bool expected =
        m_pictureResolving || (m_pending && net::Connectivity::instance().online());
    return m_streamVideo || (expected && track().video);
}

bool PlaybackController::videoLive() const
{
    return m_engine.pictureWidth() > 0 && m_engine.pictureHeight() > 0;
}

qreal PlaybackController::videoAspect() const
{
    if (!videoLive())
        return 16.0 / 9.0;
    return qreal(m_engine.pictureWidth()) / qreal(m_engine.pictureHeight());
}

void PlaybackController::setVideoShown(bool shown)
{
    if (m_videoShown == shown)
        return;
    m_videoShown = shown;
    player::VideoOutput::instance().setWanted(shown);
    Q_EMIT videoShownChanged();
}

void PlaybackController::setStreamVideo(bool available)
{
    if (m_streamVideo == available)
        return;
    m_streamVideo = available;
    Q_EMIT videoAvailableChanged();
}

player::Stream PlaybackController::asPlayed(player::Stream stream) const
{
    const int index = indexOf(stream.videoId);
    const bool queued = index >= 0 && m_queue.at(index).video;
    if (!stream.live && !stream.musicVideo.value_or(queued))
        stream.pictureUrl.clear();
    return stream;
}

void PlaybackController::resolvePicture()
{
    const Track current = track();
    if (m_streamVideo || m_pictureResolving || !m_loadedLocal || !current.video
        || m_loadedVideoId != current.videoId || !net::Connectivity::instance().online())
        return;
    setPictureResolving(true);
    m_picture.resolve(current.videoId, false, true);
}

bool PlaybackController::usesDownload(const QString &videoId) const
{
    return videoId == track().videoId || videoId == m_loadedVideoId || videoId == m_stagingVideoId
        || m_engine.fading();
}

void PlaybackController::startCurrent()
{
    finishTracking();
    const Track current = track();
    if (!current.valid())
        return;

    setError({});
    setStreamVideo(false);
    setPictureResolving(false);
    dropStaging();
    m_engine.pause();
    m_loadedVideoId.clear();
    m_currentStream = {};
    setPosition(m_resumePosition);
    const auto stored = library::Downloads::instance().localStream(current);
    diagnostics::breadcrumb("playback.start_requested",
                            {{"source", stored ? "download" : "stream"},
                             {"kind", kindOf(current)},
                             {"online", net::Connectivity::instance().online()}});
    if (!stored && !net::Connectivity::instance().online()) {
        setPending(false);
        net::Connectivity::instance().check();
        reportOffline();
        return;
    }
    setPending(true);
    if (stored) {
        applyStream(*stored);
        return;
    }
    m_resolver.resolve(current.videoId, current.upload, true);
}

bool PlaybackController::advance(int delta)
{
    if (m_queue.isEmpty())
        return false;

    const int size = m_queue.size();
    int target = m_queueIndex;
    int remaining = size;
    bool wrapped = false;
    while (remaining-- > 0) {
        target += delta;
        if (target < 0 || target >= size) {
            if (!repeating() || wrapped)
                return false;
            wrapped = true;
            remaining = size - 1;
            target = target < 0 ? size - 1 : 0;
            if (delta > 0 && m_shuffle)
                shuffleFrom(-1);
        }
        if (reachable(m_queue.at(target))) {
            selectTrack(target, delta < 0 ? -1 : 1);
            return true;
        }
    }
    return false;
}

void PlaybackController::stopAtEnd()
{
    finishTracking();
    setPosition(m_duration);
    setPending(false);
    m_engine.pause();
    const bool stranded = repeating() || m_queueIndex < m_queue.size() - 1;
    if (stranded && !net::Connectivity::instance().online())
        Q_EMIT unavailable(tr("You're offline and nothing else in the queue is downloaded."));
}

void PlaybackController::enrichCurrent(const player::Stream &stream)
{
    if (m_queueIndex < 0 || m_queueIndex >= m_queue.size())
        return;

    Track &current = m_queue[m_queueIndex];
    bool enriched = false;
    if (current.title.isEmpty() && !stream.title.isEmpty()) {
        current.title = stream.title;
        enriched = true;
    }
    if (current.artist.isEmpty() && !stream.artist.isEmpty()) {
        current.artist = stream.artist;
        enriched = true;
    }
    if (current.live != stream.live) {
        current.live = stream.live;
        current.durationMs = stream.live ? 0 : current.durationMs;
        enriched = true;
    }
    if (current.durationMs == 0 && stream.durationMs > 0) {
        current.durationMs = stream.durationMs;
        enriched = true;
    }
    if (stream.episode && *stream.episode != current.episode) {
        current.episode = *stream.episode;
        current.video = current.video || current.episode;
        enriched = true;
    }

    if (enriched) {
        Q_EMIT queueChanged();
        Q_EMIT trackChanged();
    }
    if (current.durationMs > 0 || current.live)
        setDuration(current.durationMs);
}

void PlaybackController::applyStream(const player::Stream &stream)
{
    if (stream.videoId != track().videoId || !m_pending)
        return;

    enrichCurrent(stream);
    m_loadedVideoId = stream.videoId;
    m_loadedLocal = stream.url.isLocalFile();
    m_currentStream = stream;
    if (!episode())
        requestAnalysisFor(stream);
    const player::Stream played = asPlayed(stream);
    diagnostics::breadcrumb("playback.stream_ready",
                            {{"source", m_loadedLocal ? "download" : "stream"},
                             {"client", diagnostics::Value::symbol(stream.clientKey)},
                             {"codec", codecOf(stream.mimeType)},
                             {"itag", stream.itag},
                             {"video", played.showsPicture()},
                             {"live", stream.live}});
    setStreamVideo(played.showsPicture());
    const qint64 start = startFor(stream);
    applyRate();
    m_engine.setLoopFile(m_repeat == RepeatOne && !episode());
    setPosition(start);
    m_engine.load(played, start);
    resolvePicture();
    prepareTracking(stream);
    m_resumePosition = 0;
    m_engine.play();
    library::PlayLog::instance().begin(track());
}

qint64 PlaybackController::startFor(const player::Stream &stream) const
{
    if (m_resumePosition > 0 || !episode())
        return m_resumePosition;
    const qint64 length = stream.durationMs > 0 ? stream.durationMs : m_duration;
    if (stream.startMs < kRestartThresholdMs
        || (length > 0 && stream.startMs > length - kResumeTailMs))
        return 0;
    return stream.startMs;
}

void PlaybackController::applyRate()
{
    m_engine.setRate(episode() ? player::PlaybackSettings::instance().podcastSpeed() : 1.0);
}

void PlaybackController::finishTracking()
{
    ++m_trackingGeneration;
    m_tracker.finish();
}

void PlaybackController::prepareTracking(const player::Stream &stream)
{
    finishTracking();
    auto &session = innertube::Session::instance();
    if (!session.authenticated())
        return;
    m_activeStream = stream;
    m_tracker.prepare(stream.tracking);

    if (!stream.url.isLocalFile() && !stream.tracking.playbackUrl.isEmpty())
        return;

    const auto *client = session.clients().metadataClient();
    if (!client)
        return;

    const quint64 generation = m_trackingGeneration;
    const QPointer<PlaybackController> guard(this);
    innertube::Endpoints endpoints(session);

    endpoints.player(*client, stream.videoId, track().upload ? QStringLiteral("MLPT") : QString(),
                     0, [guard, generation, client](const innertube::Reply &reply) {
        if (!guard || generation != guard->m_trackingGeneration)
            return;
        if (!reply.ok()) {
            qCDebug(logPlayback) << "playback tracking seed unavailable";
            return;
        }
        guard->m_tracker.setSeed(
            player::PlaybackTrackingSeed::fromResponse(reply.json, client->key, client->name));
    });
}

void PlaybackController::reportFailure(const QString &videoId, const QString &message,
                                       bool unreachable)
{
    if (videoId != track().videoId)
        return;

    diagnostics::breadcrumb(
        "playback.stream_failed",
        {{"reason", unreachable ? "unreachable" : "unplayable"}, {"kind", kindOf(track())}},
        diagnostics::Level::Warning);
    setPending(false);
    if (unreachable) {
        reportOffline();
        return;
    }
    qCWarning(logPlayback) << "cannot play" << videoId << message;
    setError(track().upload ? tr("YouTube would not stream this upload.")
                            : tr("This track could not be played. Try again or choose another."));
}

void PlaybackController::reportOffline()
{
    Q_EMIT unavailable(tr("You're offline and this song isn't downloaded."));
}

bool PlaybackController::reachable(const Track &entry) const
{
    return net::Connectivity::instance().online()
        || library::Downloads::instance().stateOf(entry.videoId) == library::Downloads::Ready;
}

void PlaybackController::followConnectivity()
{
    Q_EMIT videoAvailableChanged();
    if (!net::Connectivity::instance().online())
        return;
    resolvePicture();
    considerAutoplay();
    requestAnalyses();
}

void PlaybackController::handleTrackEnd()
{
    if (m_loadedVideoId.isEmpty())
        return;

    advanceQueue();
}

void PlaybackController::selectTrack(int index, int direction)
{
    finishTracking();
    library::PlayLog::instance().finish();
    const int previousOffset = queueOffset();
    m_queueIndex = index;
    m_resumePosition = 0;
    setPosition(0);
    setDuration(track().durationMs);
    if (queueOffset() != previousOffset)
        Q_EMIT queueChanged();
    Q_EMIT queueIndexChanged();
    Q_EMIT trackAdvanced(direction);
    Q_EMIT trackChanged();
    Q_EMIT reachChanged();
    m_currentEntry = {};
    library::LibraryActions::instance().refreshTrack(track().videoId);
    requestAnalyses();
    considerAutoplay();
}

void PlaybackController::requestAnalyses()
{
    const bool wanted = analysisDepth().has_value();

    QStringList targets;
    const int next = m_queueIndex + 1;
    if (wanted) {
        if (track().valid() && !episode())
            targets.append(track().videoId);
        if (m_repeat != RepeatOne && next >= 0 && next < m_queue.size()
            && !m_queue.at(next).episode)
            targets.append(m_queue.at(next).videoId);
    }
    m_analyzer.setTargets(targets);

    if (!wanted || m_queueIndex < 0 || m_queueIndex >= m_queue.size())
        return;
    requestAnalysis(m_queue.at(m_queueIndex), true);
    if (m_repeat != RepeatOne && next >= 0 && next < m_queue.size())
        requestAnalysis(m_queue.at(next), false);
}

void PlaybackController::requestAnalysisFor(const player::Stream &stream)
{
    if (stream.live)
        return;
    if (const std::optional<analysis::Analyzer::Depth> depth = analysisDepth())
        m_analyzer.request(stream, *depth);
}

void PlaybackController::requestAnalysis(const Track &entry, bool current)
{
    if (!entry.valid() || entry.live || entry.episode)
        return;

    if (const auto stored = library::Downloads::instance().localStream(entry)) {
        requestAnalysisFor(*stored);
        return;
    }
    if (!net::Connectivity::instance().online())
        return;

    const player::Stream &known = current ? m_currentStream : m_stagedStream;
    if (known.videoId == entry.videoId && known.valid()) {
        requestAnalysisFor(known);
        return;
    }
    if (current)
        m_analysisCurrent.resolve(entry.videoId, entry.upload, true);
    else
        m_analysisNext.resolve(entry.videoId, entry.upload, true);
}

int PlaybackController::stagedIndex() const
{
    if (!m_engine.staged())
        return -1;
    const int upcoming = m_queueIndex + 1;
    if (upcoming < m_queue.size() && m_queue.at(upcoming).videoId == m_engine.stagedVideoId())
        return upcoming;
    return indexOf(m_engine.stagedVideoId());
}

void PlaybackController::considerStaging()
{
    if (m_pending || m_duration <= 0 || m_repeat == RepeatOne)
        return;

    const player::PlaybackSettings &settings = player::PlaybackSettings::instance();
    if (settings.transitionMode() == player::PlaybackSettings::TransitionsOff)
        return;

    const int upcoming = m_queueIndex + 1;
    if (upcoming >= m_queue.size() || live() || episode() || m_queue.at(upcoming).live
        || m_queue.at(upcoming).episode)
        return;

    logTransitionPlan(m_queue.at(upcoming));

    const int fade = crossfadeMilliseconds();
    const std::optional<player::TransitionPlan> plan =
        executableTransitionPlan(m_queue.at(upcoming));
    if (m_stagingVideoId.isEmpty()) {
        if (plan) {
            const qint64 stageAt = qMax(qint64(0), plan->outgoingStartMs - kStageLeadMs);
            if (m_position >= stageAt)
                stage(upcoming, plan);
        } else if (m_duration - m_position <= fade + kStageLeadMs) {
            stage(upcoming);
        }
    }
    if (m_runnerStaged || m_pendingRunnerPlan)
        return;
    if (fade <= 0 || stagedIndex() != upcoming)
        return;

    std::optional<qint64> handoff;
    if (settings.transitionMode() == player::PlaybackSettings::Smart) {
        const std::optional<analysis::TrackAnalysis> outgoing =
            m_analyzer.analysis(m_currentStream.videoId, m_currentStream.itag);
        const std::optional<analysis::TrackAnalysis> incoming =
            m_analyzer.analysis(m_stagedStream.videoId, m_stagedStream.itag);
        if (outgoing && incoming) {
            const std::optional<SmartTransition> transition =
                smartTransitionFor(*outgoing, *incoming, fade);
            if (transition) {
                handoff = m_engine.scheduleSmartTransition(
                    transition->fadeStartMs, transition->stopMs, transition->incomingStartMs,
                    transition->naturalFade);
            }
        }
    }
    if (!handoff) {
        const std::optional<analysis::TrackAnalysis> outgoing =
            m_analyzer.analysis(m_currentStream.videoId, m_currentStream.itag);
        const std::optional<analysis::TrackAnalysis> incoming =
            m_analyzer.analysis(m_stagedStream.videoId, m_stagedStream.itag);
        const qint64 audibleEnd = outgoing ? outgoing->trailingSilenceStartMs : 0;
        const qint64 entry = incoming ? incoming->leadingSilenceEndMs : 0;
        handoff = m_engine.scheduleCrossfade(fade, audibleEnd, entry);
    }
    if (handoff && m_position >= *handoff)
        promoteStaged(true);
}

std::optional<player::TransitionPlan>
PlaybackController::executableTransitionPlan(const Track &upcoming) const
{
    const player::PlaybackSettings &settings = player::PlaybackSettings::instance();
    if (settings.transitionMode() != player::PlaybackSettings::Smart)
        return std::nullopt;

    const Track current = track();
    const std::optional<analysis::TrackAnalysis> outgoing = m_analyzer.analysisFor(current.videoId);
    const std::optional<analysis::TrackAnalysis> incoming =
        m_analyzer.analysisFor(upcoming.videoId);
    if (!outgoing || !incoming)
        return std::nullopt;

    player::TransitionPlanner::Context context;
    context.mode = settings.transitionMode();
    context.lengthCapMs = crossfadeMilliseconds();
    context.outgoingDurationMs = m_duration;
    context.matchTempo = settings.matchTempo();
    context.sameAlbum = !m_shuffle && !current.album.isEmpty() && current.album == upcoming.album
        && current.artist == upcoming.artist;

    const player::TransitionPlan plan =
        player::TransitionPlanner::plan(outgoing, incoming, context);
    if (!plan.valid() || plan.kind == player::TransitionPlan::Kind::Fixed)
        return std::nullopt;
    return plan;
}

void PlaybackController::logTransitionPlan(const Track &upcoming)
{
    const Track current = track();
    if (!current.valid() || !upcoming.valid())
        return;

    const player::PlaybackSettings &settings = player::PlaybackSettings::instance();
    const std::optional<analysis::TrackAnalysis> outgoing = m_analyzer.analysisFor(current.videoId);
    const std::optional<analysis::TrackAnalysis> incoming =
        m_analyzer.analysisFor(upcoming.videoId);

    player::TransitionPlanner::Context context;
    context.mode = settings.transitionMode();
    context.lengthCapMs = crossfadeMilliseconds();
    context.outgoingDurationMs = m_duration;
    context.matchTempo = settings.matchTempo();
    context.sameAlbum = !m_shuffle && !current.album.isEmpty() && current.album == upcoming.album
        && current.artist == upcoming.artist;

    const QString pair = QStringLiteral("%1|%2|%3|%4|%5|%6|%7|%8")
                             .arg(current.videoId, upcoming.videoId)
                             .arg(int(context.mode))
                             .arg(context.lengthCapMs)
                             .arg(context.outgoingDurationMs)
                             .arg(context.matchTempo ? 1 : 0)
                             .arg(context.sameAlbum ? 1 : 0)
                             .arg((outgoing ? 2 : 0) + (incoming ? 1 : 0));
    if (m_plannedPair == pair)
        return;
    m_plannedPair = pair;

    const player::TransitionPlan plan =
        player::TransitionPlanner::plan(outgoing, incoming, context);
    qCInfo(logTransition) << "plan" << qUtf8Printable(plan.kindName()) << "a" << current.videoId
                          << "b" << upcoming.videoId << qUtf8Printable(plan.describe())
                          << qUtf8Printable(plan.trace.join(QLatin1Char(' ')));
}

void PlaybackController::considerAutoplay()
{
    if (!player::PlaybackSettings::instance().autoplay() || m_repeat != RepeatOff || episode()
        || !net::Connectivity::instance().online())
        return;
    if (m_queue.isEmpty() || m_queueIndex < m_queue.size() - kAutoplayLead)
        return;

    m_radio.extend(m_queue.constLast().videoId);
}

void PlaybackController::stage(int index, const std::optional<player::TransitionPlan> &plan)
{
    const Track &upcoming = m_queue.at(index);
    if (m_stagingVideoId == upcoming.videoId)
        return;

    const auto stored = library::Downloads::instance().localStream(upcoming);
    if (!stored && !net::Connectivity::instance().online())
        return;
    m_stagingVideoId = upcoming.videoId;
    m_pendingRunnerPlan = plan;
    if (stored) {
        m_stagedStream = asPlayed(*stored);
        requestAnalysisFor(m_stagedStream);
        const std::optional<player::TransitionPlan> pending = m_pendingRunnerPlan;
        m_pendingRunnerPlan.reset();
        m_runnerStaged = pending && m_engine.stage(m_stagedStream, *pending);
        if (!m_runnerStaged)
            m_engine.stage(m_stagedStream);
        return;
    }
    m_prefetch.resolve(upcoming.videoId, upcoming.upload, true);
}

void PlaybackController::dropStaging()
{
    if (m_stagingVideoId.isEmpty() && !m_runnerStaged && !m_pendingRunnerPlan)
        return;

    m_stagingVideoId.clear();
    m_stagedStream = {};
    m_pendingRunnerPlan.reset();
    m_runnerStaged = false;
    m_engine.discardStaged();
}

void PlaybackController::dropTransition()
{
    dropStaging();
    m_engine.endTransition();
}

void PlaybackController::promoteStaged(bool crossfade)
{
    diagnostics::breadcrumb(
        "playback.transition_promoted",
        {{"mode", transitionModeName(player::PlaybackSettings::instance().transitionMode())},
         {"crossfade", crossfade},
         {"runner", m_runnerStaged}});
    const player::Stream stream = m_stagedStream;
    m_stagingVideoId.clear();
    m_stagedStream = {};
    m_pendingRunnerPlan.reset();

    setError({});
    m_currentStream = stream;
    selectTrack(m_queueIndex + 1, 1);
    m_loadedVideoId = stream.videoId;
    m_loadedLocal = stream.url.isLocalFile();
    setStreamVideo(stream.showsPicture());
    setPictureResolving(false);
    enrichCurrent(stream);
    setPending(false);
    prepareTracking(stream);
    m_engine.promoteStaged(crossfade);
    m_runnerStaged = false;
    resolvePicture();
    library::PlayLog::instance().begin(track());
}

void PlaybackController::setPosition(qint64 milliseconds)
{
    if (m_position == milliseconds)
        return;
    m_position = milliseconds;
    Q_EMIT positionChanged();
}

void PlaybackController::setDuration(qint64 milliseconds)
{
    if (m_duration == milliseconds)
        return;
    m_duration = milliseconds;
    Q_EMIT durationChanged();
}

void PlaybackController::setPending(bool pending)
{
    if (m_pending == pending)
        return;
    m_pending = pending;
    Q_EMIT videoAvailableChanged();
    updatePlaying();
}

void PlaybackController::setPictureResolving(bool resolving)
{
    if (m_pictureResolving == resolving)
        return;
    m_pictureResolving = resolving;
    Q_EMIT videoAvailableChanged();
}

void PlaybackController::setError(const QString &error)
{
    if (m_error == error)
        return;
    m_error = error;
    Q_EMIT errorChanged();
}

void PlaybackController::updatePlaying()
{
    const bool playing = m_engine.playing() || m_pending;
    if (m_playing == playing)
        return;

    m_playing = playing;
    qCDebug(logPlayback) << (playing ? "play" : "pause") << track().videoId;
    Q_EMIT playingChanged();
}

}
