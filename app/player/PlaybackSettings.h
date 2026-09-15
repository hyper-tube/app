#pragma once

#include <QList>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QStringList>

namespace player {

class PlaybackSettings : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int crossfadeSeconds READ crossfadeSeconds WRITE setCrossfadeSeconds NOTIFY
                   crossfadeChanged)
    Q_PROPERTY(int maximumCrossfadeSeconds READ maximumCrossfadeSeconds CONSTANT)
    Q_PROPERTY(TransitionMode transitionMode READ transitionMode WRITE setTransitionMode NOTIFY
                   transitionModeChanged)
    Q_PROPERTY(bool matchTempo READ matchTempo WRITE setMatchTempo NOTIFY matchTempoChanged)
    Q_PROPERTY(bool equalizerEnabled READ equalizerEnabled WRITE setEqualizerEnabled NOTIFY
                   equalizerChanged)
    Q_PROPERTY(bool normalizeLoudness READ normalizeLoudness WRITE setNormalizeLoudness NOTIFY
                   equalizerChanged)
    Q_PROPERTY(bool autoplay READ autoplay WRITE setAutoplay NOTIFY autoplayChanged)
    Q_PROPERTY(bool playVideos READ playVideos WRITE setPlayVideos NOTIFY playVideosChanged)
    Q_PROPERTY(
        double podcastSpeed READ podcastSpeed WRITE setPodcastSpeed NOTIFY podcastSpeedChanged)
    Q_PROPERTY(QList<double> podcastSpeeds READ podcastSpeeds CONSTANT)
    Q_PROPERTY(QList<int> gains READ gains NOTIFY equalizerChanged)
    Q_PROPERTY(int preset READ preset WRITE setPreset NOTIFY equalizerChanged)
    Q_PROPERTY(int gainRange READ gainRange CONSTANT)
    Q_PROPERTY(QStringList bands READ bands CONSTANT)
    Q_PROPERTY(QStringList presets READ presets NOTIFY presetsChanged)

public:
    enum TransitionMode {
        TransitionsOff,
        Crossfade,
        Smart,
    };
    Q_ENUM(TransitionMode)

    explicit PlaybackSettings(QObject *parent);

    static PlaybackSettings &instance();
    static PlaybackSettings *create(QQmlEngine *, QJSEngine *);

    int crossfadeSeconds() const { return m_crossfadeSeconds; }
    int maximumCrossfadeSeconds() const;
    TransitionMode transitionMode() const { return m_transitionMode; }
    bool matchTempo() const { return m_matchTempo; }
    bool equalizerEnabled() const { return m_equalizerEnabled; }
    bool normalizeLoudness() const { return m_normalizeLoudness; }
    bool autoplay() const { return m_autoplay; }
    bool playVideos() const { return m_playVideos; }
    double podcastSpeed() const { return m_podcastSpeed; }
    QList<double> podcastSpeeds() const;
    const QList<int> &gains() const { return m_gains; }
    int preset() const { return m_preset; }
    int gainRange() const;
    QStringList bands() const;
    QStringList presets() const;

    void setCrossfadeSeconds(int seconds);
    void setTransitionMode(TransitionMode mode);
    void setMatchTempo(bool match);
    void setEqualizerEnabled(bool enabled);
    void setNormalizeLoudness(bool normalize);
    void setAutoplay(bool autoplay);
    void setPlayVideos(bool play);
    void setPodcastSpeed(double speed);
    void setPreset(int preset);

    Q_INVOKABLE void setGain(int band, int decibels);
    Q_INVOKABLE void resetEqualizer();

    QString filterChain() const;

Q_SIGNALS:
    void crossfadeChanged();
    void transitionModeChanged();
    void matchTempoChanged();
    void equalizerChanged();
    void autoplayChanged();
    void playVideosChanged();
    void podcastSpeedChanged();
    void presetsChanged();

private:
    void save() const;
    void refreshPreset();

    QList<int> m_gains;
    int m_crossfadeSeconds = 0;
    int m_preset = 0;
    double m_podcastSpeed = 1.0;
    TransitionMode m_transitionMode = TransitionsOff;
    bool m_matchTempo = true;
    bool m_equalizerEnabled = false;
    bool m_normalizeLoudness = true;
    bool m_autoplay = true;
    bool m_playVideos = false;
};

}
