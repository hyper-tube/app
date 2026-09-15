#pragma once

#include "Radio.h"
#include "Track.h"
#include "analysis/Analyzer.h"
#include "model/Item.h"
#include "innertube/Session.h"
#include "player/AudioEngine.h"
#include "player/PlaybackTracker.h"
#include "player/StreamResolver.h"

#include <QList>
#include <QObject>
#include <QQmlEngine>

#include <optional>

namespace media {

class PlaybackController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(media::Track track READ track NOTIFY trackChanged)
    Q_PROPERTY(QList<media::Track> queue READ queue NOTIFY queueChanged)
    Q_PROPERTY(model::Item queueSource READ queueSource NOTIFY queueSourceChanged)
    Q_PROPERTY(QList<media::Track> queueWindow READ queueWindow NOTIFY queueChanged)
    Q_PROPERTY(int queueOffset READ queueOffset NOTIFY queueChanged)
    Q_PROPERTY(int queueIndex READ queueIndex NOTIFY queueIndexChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(bool live READ live NOTIFY trackChanged)
    Q_PROPERTY(bool episode READ episode NOTIFY trackChanged)
    Q_PROPERTY(bool shuffle READ shuffle WRITE setShuffle NOTIFY shuffleChanged)
    Q_PROPERTY(RepeatMode repeat READ repeat WRITE setRepeat NOTIFY repeatChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(bool canGoNext READ canGoNext NOTIFY reachChanged)
    Q_PROPERTY(bool canGoPrevious READ canGoPrevious NOTIFY reachChanged)
    Q_PROPERTY(bool videoAvailable READ videoAvailable NOTIFY videoAvailableChanged)
    Q_PROPERTY(bool videoShown READ videoShown WRITE setVideoShown NOTIFY videoShownChanged)
    Q_PROPERTY(bool videoLive READ videoLive NOTIFY videoPictureChanged)
    Q_PROPERTY(qreal videoAspect READ videoAspect NOTIFY videoPictureChanged)

public:
    enum RepeatMode {
        RepeatOff,
        RepeatAll,
        RepeatOne,
    };
    Q_ENUM(RepeatMode)

    explicit PlaybackController(QObject *parent);

    static PlaybackController &instance();
    static PlaybackController *create(QQmlEngine *, QJSEngine *);

    Track track() const;
    const QList<Track> &queue() const { return m_queue; }
    const model::Item &queueSource() const { return m_queueSource; }
    QList<Track> queueWindow() const;
    int queueOffset() const;
    int queueIndex() const { return m_queueIndex; }
    bool resolving() const
    {
        return m_pending || (!m_stagingVideoId.isEmpty() && !m_stagedStream.valid());
    }
    bool usesDownload(const QString &videoId) const;
    bool playing() const { return m_playing; }
    QString error() const { return m_error; }
    qint64 position() const { return m_position; }
    qint64 duration() const { return m_duration; }
    bool live() const { return track().live; }
    bool episode() const { return track().episode && !track().live; }
    bool shuffle() const { return m_shuffle; }
    RepeatMode repeat() const { return m_repeat; }
    int volume() const { return m_volume; }
    bool muted() const { return m_muted; }
    bool canGoNext() const;
    bool canGoPrevious() const;
    bool videoAvailable() const;
    bool videoShown() const { return m_videoShown; }
    bool videoLive() const;
    qreal videoAspect() const;

    void setShuffle(bool shuffle);
    void setVideoShown(bool shown);
    void setRepeat(RepeatMode repeat);
    void setVolume(int volume);
    void setMuted(bool muted);
    void restoreQueue(const QList<media::Track> &tracks, int index, const model::Item &source);
    void continueRemoteQueue(const QList<media::Track> &tracks, int index,
                             const model::Item &source);
    void resumeAt(qint64 milliseconds);

    Q_INVOKABLE void setQueue(const QList<media::Track> &tracks, int index,
                              const model::Item &source = {});
    Q_INVOKABLE void playQueue(const QList<media::Track> &tracks, int index,
                               const model::Item &source = {});
    Q_INVOKABLE void clearQueue();
    Q_INVOKABLE void playNext(const QList<media::Track> &tracks);
    Q_INVOKABLE void enqueue(const QList<media::Track> &tracks);
    Q_INVOKABLE void removeFromQueue(int index);
    Q_INVOKABLE model::Item queueEntry(int index) const;
    Q_INVOKABLE model::Item currentEntry() const;
    Q_INVOKABLE void repeatCurrent(bool next);
    Q_INVOKABLE void playVideoIds(const QString &input);
    Q_INVOKABLE void playAt(int index);
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void toggle();
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void seek(qint64 milliseconds);
    Q_INVOKABLE void skip(qint64 milliseconds);
    Q_INVOKABLE void cycleRepeat();
    Q_INVOKABLE void toggleMuted();
    Q_INVOKABLE void toggleLike() const;
    Q_INVOKABLE static QString formatTime(qint64 milliseconds);

Q_SIGNALS:
    void trackChanged();
    void queueChanged();
    void queueSourceChanged();
    void remoteQueueContinued();
    void queueIndexChanged();
    void playingChanged();
    void errorChanged();
    void positionChanged();
    void durationChanged();
    void shuffleChanged();
    void repeatChanged();
    void volumeChanged();
    void mutedChanged();
    void reachChanged();
    void trackAdvanced(int direction);
    void seeked(qint64 milliseconds);
    void unavailable(const QString &message);
    void videoAvailableChanged();
    void videoShownChanged();
    void videoPictureChanged();

private:
    void adoptQueue(const QList<Track> &tracks, int index, const model::Item &source);
    void removeCurrent();
    void setQueueSource(const model::Item &source);
    void shuffleFrom(int keep);
    void restoreOrder();
    void appendTracks(const QList<Track> &tracks);
    void insertTracks(const QList<Track> &tracks, bool next);
    QList<Track> unknownTracks(const QList<Track> &tracks) const;
    int indexOf(const QString &videoId) const;

    void startCurrent();
    void enrichCurrent(const player::Stream &stream);
    bool advance(int delta);
    void advanceQueue();
    void stopAtEnd();
    void applyStream(const player::Stream &stream);
    void prepareTracking(const player::Stream &stream);
    void finishTracking();
    void reportFailure(const QString &videoId, const QString &message, bool unreachable);
    void reportOffline();
    void followConnectivity();
    bool reachable(const Track &entry) const;
    void handleTrackEnd();
    void selectTrack(int index, int direction);
    void considerStaging();
    void logTransitionPlan(const Track &upcoming);
    std::optional<player::TransitionPlan> executableTransitionPlan(const Track &upcoming) const;
    void considerAutoplay();
    void requestAnalyses();
    void requestAnalysisFor(const player::Stream &stream);
    void requestAnalysis(const Track &entry, bool current);
    void stage(int index, const std::optional<player::TransitionPlan> &plan = {});
    void dropStaging();
    void dropTransition();
    void promoteStaged(bool crossfade);
    int stagedIndex() const;
    void setPosition(qint64 milliseconds);
    void setDuration(qint64 milliseconds);
    void setPending(bool pending);
    void setError(const QString &error);
    void setStreamVideo(bool available);
    void resolvePicture();
    void setPictureResolving(bool resolving);
    void updatePlaying();
    void applyRate();
    bool repeating() const;
    qint64 startFor(const player::Stream &stream) const;
    player::Stream asPlayed(player::Stream stream) const;

    analysis::Analyzer m_analyzer;
    player::StreamResolver m_resolver;
    player::StreamResolver m_prefetch;
    player::StreamResolver m_picture;
    player::StreamResolver m_analysisCurrent;
    player::StreamResolver m_analysisNext;
    player::PlaybackTracker m_tracker;
    player::AudioEngine m_engine;
    Radio m_radio;
    QList<Track> m_queue;
    QList<Track> m_unshuffled;
    model::Item m_queueSource;
    model::Item m_currentEntry;
    player::Stream m_stagedStream;
    player::Stream m_currentStream;
    std::optional<player::TransitionPlan> m_pendingRunnerPlan;
    player::Stream m_activeStream;
    quint64 m_trackingGeneration = 0;
    QString m_loadedVideoId;
    QString m_stagingVideoId;
    QString m_plannedPair;
    QString m_error;
    int m_queueIndex = -1;
    qint64 m_position = 0;
    qint64 m_duration = 0;
    qint64 m_resumePosition = 0;
    int m_volume = 100;
    bool m_muted = false;
    bool m_playing = false;
    bool m_pending = false;
    bool m_shuffle = false;
    bool m_videoShown = false;
    bool m_streamVideo = false;
    bool m_pictureResolving = false;
    bool m_loadedLocal = false;
    bool m_runnerStaged = false;
    RepeatMode m_repeat = RepeatOff;
};

}
