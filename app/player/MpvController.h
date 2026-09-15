#pragma once

#include "Fade.h"

#include <QByteArray>
#include <QElapsedTimer>
#include <QList>
#include <QObject>
#include <QRect>
#include <QSize>
#include <QString>
#include <QTimer>
#include <QUrl>

#include <optional>

struct mpv_handle;
struct mpv_event;
struct mpv_event_property;

namespace player {

class MpvController : public QObject
{
    Q_OBJECT

public:
    explicit MpvController(QObject *parent = nullptr);
    ~MpvController() override;

    bool playing() const { return m_playing; }
    bool loaded() const { return m_loaded; }
    int volume() const { return m_volume; }
    bool muted() const { return m_muted; }
    qint64 position() const { return m_position; }
    qint64 duration() const { return m_duration; }
    double audioPts() const;
    double speed() const { return m_speed; }
    bool hasAudioPts() const { return m_hasAudioPts; }
    mpv_handle *handle() const { return m_handle; }
    bool hasPicture() const { return !m_pictureUrl.isEmpty(); }
    int pictureWidth() const { return m_pictureWidth; }
    int pictureHeight() const { return m_pictureHeight; }
    const Fade &fade() const { return m_fade; }

    void load(const QUrl &url, const QString &userAgent, const QString &cookie,
              qint64 startMilliseconds, bool paused);
    void play();
    void pause();
    void stop();
    void seek(qint64 milliseconds);
    void setLoopFile(bool loop);
    void setVolume(int volume);
    void setMuted(bool muted);
    void setTrackGain(std::optional<double> gainDb);
    void setFade(const Fade &fade);
    void setLowShelf(double gainDb);
    void setStretcher(bool enabled);
    void setTransitionFilters(bool enabled);
    void clearFadeAndShelf();
    void setSpeed(double speed);
    void resetTransition();
    void forgetTransition();
    void refreshFilters();
    void setPictureUrl(const QUrl &url);
    void setPictureEnabled(bool enabled);

Q_SIGNALS:
    void positionChanged(qint64 milliseconds);
    void durationChanged(qint64 milliseconds);
    void playingChanged(bool playing);
    void loadedChanged(bool loaded);
    void trackEnded();
    void discontinuity();
    void ending();
    void failed(const QString &message);
    void filterChainFailed();
    void pictureSizeChanged();

private:
    enum class Request : quint64 {
        Generic,
        Load,
        Stop,
        Seek,
        Filters,
        Speed,
        Start,
        Volume,
        Mute,
        Pause,
        LoopFile,
        UserAgent,
        HttpHeaders,
        LowShelf,
        PictureAdd,
        PictureRemove,
        TrimFilter,
        TrimCrop,
        TrimMetadata,
    };

    static void wakeup(void *context);
    static const char *requestName(Request request);

    void drainEvents();
    void handleEvent(mpv_event *event);
    void handleProperty(mpv_event_property *property);
    void handleReply(Request request, int status);
    void handleValue(Request request, const mpv_event_property *property);
    void updatePlaying();
    void setLoaded(bool loaded);
    void syncPictureTrack();
    void beginTrimming();
    void endTrimming();
    void sampleTrim();
    void applyTrim(const QRect &box);
    void setPictureSize(int width, int height);
    void setStart(qint64 milliseconds);
    void applyFilters();
    void applyVolume();
    void command(const QList<QByteArray> &arguments, Request request);
    void readProperty(const char *name, int format, Request request);
    void setProperty(const char *name, const QString &value, Request request);
    void setFlag(const char *name, bool value, Request request);

    mpv_handle *m_handle = nullptr;
    QTimer m_trimTimer;
    QElapsedTimer m_audioPtsClock;
    QRect m_trimBox;
    QSize m_trimSource;
    QUrl m_pictureUrl;
    std::optional<double> m_trackGainDb;
    Fade m_fade;
    double m_lowShelfDb = 0.0;
    double m_speed = 1.0;
    double m_audioPts = 0.0;
    qint64 m_position = 0;
    qint64 m_duration = 0;
    int m_volume = 100;
    int m_pictureWidth = 0;
    int m_pictureHeight = 0;
    int m_trimStreak = 0;
    bool m_pictureEnabled = false;
    bool m_pictureAttached = false;
    bool m_muted = false;
    bool m_paused = false;
    bool m_idle = true;
    bool m_playing = false;
    bool m_loaded = false;
    bool m_seeking = false;
    bool m_buffering = false;
    bool m_hasAudioPts = false;
    bool m_stretcher = false;
    bool m_transitionFilters = false;
};

}
