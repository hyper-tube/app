#include "PlaybackSettings.h"

#include "core/Localization.h"

#include <QCoreApplication>
#include <QSettings>
#include <QStringList>

namespace {

struct Band
{
    int frequency;
    const char *label;
};

struct Preset
{
    const char *name;
    int gains[10];
};

constexpr Band kBands[] = {
    {32, "32"},   {64, "64"},   {125, "125"}, {250, "250"}, {500, "500"},
    {1000, "1k"}, {2000, "2k"}, {4000, "4k"}, {8000, "8k"}, {16000, "16k"},
};

constexpr int kBandCount = int(std::size(kBands));

constexpr Preset kPresets[] = {
    {QT_TRANSLATE_NOOP("player::PlaybackSettings", "Flat"), {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    {QT_TRANSLATE_NOOP("player::PlaybackSettings", "Bass boost"), {6, 5, 4, 2, 0, 0, 0, 0, 0, 0}},
    {QT_TRANSLATE_NOOP("player::PlaybackSettings", "Loudness"), {5, 4, 2, 0, -1, -1, 0, 2, 4, 5}},
    {QT_TRANSLATE_NOOP("player::PlaybackSettings", "Vocal"), {-3, -2, 0, 2, 4, 4, 3, 1, 0, -1}},
    {QT_TRANSLATE_NOOP("player::PlaybackSettings", "Treble boost"), {0, 0, 0, 0, 0, 1, 2, 4, 5, 6}},
    {QT_TRANSLATE_NOOP("player::PlaybackSettings", "Podcast"), {-4, -2, 0, 3, 4, 3, 2, 0, -1, -2}},
};

constexpr int kPresetCount = int(std::size(kPresets));
constexpr int kGainRange = 12;
constexpr int kMaximumCrossfadeSeconds = 20;
constexpr int kCustomPreset = -1;
constexpr double kBandWidthOctaves = 1.0;

const QString kCrossfadeKey = QStringLiteral("audio/crossfade");
const QString kTransitionModeKey = QStringLiteral("audio/transitionMode");
const QString kMatchTempoKey = QStringLiteral("audio/matchTempo");
const QString kEqualizerKey = QStringLiteral("audio/equalizer");
const QString kNormalizeKey = QStringLiteral("audio/normalize");
const QString kAutoplayKey = QStringLiteral("audio/autoplay");
const QString kPlayVideosKey = QStringLiteral("playback/videos");
const QString kPodcastSpeedKey = QStringLiteral("playback/podcastSpeed");
const QList<double> kPodcastSpeeds {0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0};
const QString kGainsKey = QStringLiteral("audio/gains");

QList<int> flatGains()
{
    return QList<int>(kBandCount, 0);
}

}

namespace player {

PlaybackSettings::PlaybackSettings(QObject *parent)
    : QObject(parent)
    , m_gains(flatGains())
{
    const QSettings settings;
    m_crossfadeSeconds = qBound(0, settings.value(kCrossfadeKey).toInt(), kMaximumCrossfadeSeconds);
    if (settings.contains(kTransitionModeKey)) {
        m_transitionMode = static_cast<TransitionMode>(
            qBound(int(TransitionsOff), settings.value(kTransitionModeKey).toInt(), int(Smart)));
    } else if (m_crossfadeSeconds > 0) {
        m_transitionMode = Crossfade;
    }
    m_matchTempo = settings.value(kMatchTempoKey, true).toBool();
    m_equalizerEnabled = settings.value(kEqualizerKey, false).toBool();
    m_normalizeLoudness = settings.value(kNormalizeKey, true).toBool();
    m_autoplay = settings.value(kAutoplayKey, true).toBool();
    m_playVideos = settings.value(kPlayVideosKey, false).toBool();
    m_podcastSpeed =
        qBound(kPodcastSpeeds.constFirst(), settings.value(kPodcastSpeedKey, 1.0).toDouble(),
               kPodcastSpeeds.constLast());

    const QStringList stored = settings.value(kGainsKey).toStringList();
    for (int band = 0; band < kBandCount && band < stored.size(); ++band)
        m_gains[band] = qBound(-kGainRange, stored.at(band).toInt(), kGainRange);

    refreshPreset();

    connect(&core::Localization::instance(), &core::Localization::resolvedChanged, this,
            &PlaybackSettings::presetsChanged);
}

PlaybackSettings &PlaybackSettings::instance()
{
    static auto *settings = new PlaybackSettings(QCoreApplication::instance());
    return *settings;
}

PlaybackSettings *PlaybackSettings::create(QQmlEngine *, QJSEngine *)
{
    QQmlEngine::setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
    return &instance();
}

int PlaybackSettings::maximumCrossfadeSeconds() const
{
    return kMaximumCrossfadeSeconds;
}

int PlaybackSettings::gainRange() const
{
    return kGainRange;
}

QStringList PlaybackSettings::bands() const
{
    QStringList labels;
    labels.reserve(kBandCount);
    for (const Band &band : kBands)
        labels.append(QString::fromLatin1(band.label));
    return labels;
}

QStringList PlaybackSettings::presets() const
{
    QStringList names;
    names.reserve(kPresetCount);
    for (const Preset &preset : kPresets)
        names.append(tr(preset.name));
    return names;
}

void PlaybackSettings::setCrossfadeSeconds(int seconds)
{
    const int bounded = qBound(0, seconds, kMaximumCrossfadeSeconds);
    if (m_crossfadeSeconds == bounded)
        return;
    m_crossfadeSeconds = bounded;
    save();
    Q_EMIT crossfadeChanged();
}

void PlaybackSettings::setTransitionMode(TransitionMode mode)
{
    if (mode < TransitionsOff || mode > Smart || m_transitionMode == mode)
        return;
    m_transitionMode = mode;
    save();
    Q_EMIT transitionModeChanged();
}

void PlaybackSettings::setMatchTempo(bool match)
{
    if (m_matchTempo == match)
        return;
    m_matchTempo = match;
    save();
    Q_EMIT matchTempoChanged();
}

void PlaybackSettings::setEqualizerEnabled(bool enabled)
{
    if (m_equalizerEnabled == enabled)
        return;
    m_equalizerEnabled = enabled;
    save();
    Q_EMIT equalizerChanged();
}

void PlaybackSettings::setNormalizeLoudness(bool normalize)
{
    if (m_normalizeLoudness == normalize)
        return;
    m_normalizeLoudness = normalize;
    save();
    Q_EMIT equalizerChanged();
}

void PlaybackSettings::setAutoplay(bool autoplay)
{
    if (m_autoplay == autoplay)
        return;
    m_autoplay = autoplay;
    save();
    Q_EMIT autoplayChanged();
}

void PlaybackSettings::setPlayVideos(bool play)
{
    if (m_playVideos == play)
        return;
    m_playVideos = play;
    save();
    Q_EMIT playVideosChanged();
}

QList<double> PlaybackSettings::podcastSpeeds() const
{
    return kPodcastSpeeds;
}

void PlaybackSettings::setPodcastSpeed(double speed)
{
    const double bounded = qBound(kPodcastSpeeds.constFirst(), speed, kPodcastSpeeds.constLast());
    if (qFuzzyCompare(m_podcastSpeed, bounded))
        return;
    m_podcastSpeed = bounded;
    save();
    Q_EMIT podcastSpeedChanged();
}

void PlaybackSettings::setPreset(int preset)
{
    if (preset < 0 || preset >= kPresetCount || m_preset == preset)
        return;

    m_preset = preset;
    for (int band = 0; band < kBandCount; ++band)
        m_gains[band] = kPresets[preset].gains[band];
    save();
    Q_EMIT equalizerChanged();
}

void PlaybackSettings::setGain(int band, int decibels)
{
    if (band < 0 || band >= kBandCount)
        return;

    const int bounded = qBound(-kGainRange, decibels, kGainRange);
    if (m_gains.at(band) == bounded)
        return;

    m_gains[band] = bounded;
    refreshPreset();
    save();
    Q_EMIT equalizerChanged();
}

void PlaybackSettings::resetEqualizer()
{
    setPreset(0);
}

QString PlaybackSettings::filterChain() const
{
    if (!m_equalizerEnabled)
        return {};

    QStringList filters;
    for (int band = 0; band < kBandCount; ++band) {
        if (m_gains.at(band) == 0)
            continue;
        filters.append(QStringLiteral("equalizer=f=%1:width_type=o:width=%2:g=%3")
                           .arg(kBands[band].frequency)
                           .arg(kBandWidthOctaves, 0, 'f', 1)
                           .arg(m_gains.at(band)));
    }
    return filters.join(QLatin1Char(','));
}

void PlaybackSettings::refreshPreset()
{
    for (int preset = 0; preset < kPresetCount; ++preset) {
        bool matches = true;
        for (int band = 0; band < kBandCount && matches; ++band)
            matches = m_gains.at(band) == kPresets[preset].gains[band];
        if (matches) {
            m_preset = preset;
            return;
        }
    }
    m_preset = kCustomPreset;
}

void PlaybackSettings::save() const
{
    QStringList stored;
    stored.reserve(m_gains.size());
    for (int const gain : m_gains)
        stored.append(QString::number(gain));

    QSettings settings;
    settings.setValue(kCrossfadeKey, m_crossfadeSeconds);
    settings.setValue(kTransitionModeKey, m_transitionMode);
    settings.setValue(kMatchTempoKey, m_matchTempo);
    settings.setValue(kEqualizerKey, m_equalizerEnabled);
    settings.setValue(kNormalizeKey, m_normalizeLoudness);
    settings.setValue(kAutoplayKey, m_autoplay);
    settings.setValue(kPlayVideosKey, m_playVideos);
    settings.setValue(kPodcastSpeedKey, m_podcastSpeed);
    settings.setValue(kGainsKey, stored);
}

}
