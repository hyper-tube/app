#include "MpvController.h"

#include "PlaybackSettings.h"
#include "core/Logging.h"
#include "core/Paths.h"

#include <QDir>
#include <QStringList>
#include <QVarLengthArray>

#include <mpv/client.h>

#include <clocale>
#include <cmath>

namespace {

constexpr const char *kDemuxerMaxBytes = "32MiB";
constexpr const char *kDemuxerMaxBackBytes = "8MiB";
constexpr const char *kBoundedRequests = "request_size=10485760,multiple_requests=1";
constexpr double kSilenceDb = -96.0;
constexpr int kTrimIntervalMs = 900;
constexpr int kTrimSamples = 4;
constexpr double kTrimSmallest = 0.04;
constexpr double kTrimLargest = 0.40;
constexpr double kRestingShelfDb = 0.01;
constexpr double kAudioPtsReachMs = 250.0;

const QString kTrimFilter = QStringLiteral("@trim:lavfi=[cropdetect=limit=24:round=2:reset=0]");

bool worthTrimming(const QRect &box, const QSize &source)
{
    if (box.width() > source.width() || box.height() > source.height())
        return false;

    const double horizontal = 1.0 - double(box.width()) / double(source.width());
    const double vertical = 1.0 - double(box.height()) / double(source.height());
    if (horizontal > kTrimLargest || vertical > kTrimLargest)
        return false;
    return horizontal >= kTrimSmallest || vertical >= kTrimSmallest;
}
constexpr double kTaperExponent = 1.5;

double gainDbForVolume(int volume)
{
    if (volume <= 0)
        return kSilenceDb;
    return qMax(kSilenceDb, 20.0 * kTaperExponent * std::log10(volume / 100.0));
}

double volumePercentForGain(double gainDb)
{
    return 100.0 * std::pow(10.0, gainDb / 20.0);
}

constexpr player::Fade kDormantFade {player::Fade::Out, 365LL * 24 * 60 * 60 * 1000, 1000};

QString fadeFilter(const player::Fade &fade)
{
    const player::Fade &scheduled = fade.active() ? fade : kDormantFade;
    return QStringLiteral("@fade:lavfi=[afade=t=%1:st=%2:d=%3:curve=qsin]")
        .arg(scheduled.direction == player::Fade::In ? QStringLiteral("in") : QStringLiteral("out"))
        .arg(double(scheduled.startMs) / 1000.0, 0, 'f', 3)
        .arg(double(scheduled.lengthMs) / 1000.0, 0, 'f', 3);
}

void ensureNumericLocaleForMpv()
{
    std::setlocale(LC_NUMERIC, "C");
}

QString streamCacheDir()
{
    const QString path = core::paths::cacheDir() + QStringLiteral("/stream");
    QDir().mkpath(path);
    return path;
}

QRect contentBox(const mpv_node &node)
{
    if (node.format != MPV_FORMAT_NODE_MAP)
        return {};

    int width = -1;
    int height = -1;
    int left = -1;
    int top = -1;
    for (int entry = 0; entry < node.u.list->num; ++entry) {
        const mpv_node &value = node.u.list->values[entry];
        if (value.format != MPV_FORMAT_STRING)
            continue;
        const QLatin1String key(node.u.list->keys[entry]);
        const int number = QString::fromUtf8(value.u.string).toInt();
        if (key == QLatin1String("lavfi.cropdetect.w"))
            width = number;
        else if (key == QLatin1String("lavfi.cropdetect.h"))
            height = number;
        else if (key == QLatin1String("lavfi.cropdetect.x"))
            left = number;
        else if (key == QLatin1String("lavfi.cropdetect.y"))
            top = number;
    }

    if (width <= 0 || height <= 0 || left < 0 || top < 0)
        return {};
    return {left, top, width, height};
}

void report(int status, const char *what)
{
    if (status < 0)
        qCWarning(logPlayback) << what << mpv_error_string(status);
}

}

namespace player {

const char *MpvController::requestName(Request request)
{
    switch (request) {
    case Request::Load: return "loadfile";
    case Request::Stop: return "stop";
    case Request::Seek: return "seek";
    case Request::Filters: return "af";
    case Request::Speed: return "speed";
    case Request::Start: return "start";
    case Request::Volume: return "volume";
    case Request::Mute: return "mute";
    case Request::Pause: return "pause";
    case Request::LoopFile: return "loop-file";
    case Request::UserAgent: return "user-agent";
    case Request::HttpHeaders: return "http-header-fields";
    case Request::LowShelf: return "low shelf";
    case Request::PictureAdd: return "video-add";
    case Request::PictureRemove: return "video-remove";
    case Request::TrimFilter: return "vf";
    case Request::TrimCrop: return "video-crop";
    case Request::TrimMetadata: return "vf-metadata/trim";
    case Request::Generic: break;
    }
    return "mpv";
}

MpvController::MpvController(QObject *parent)
    : QObject(parent)
{
    ensureNumericLocaleForMpv();

    m_handle = mpv_create();
    if (!m_handle) {
        qCCritical(logPlayback) << "libmpv is unavailable, playback is disabled";
        return;
    }

    mpv_set_option_string(m_handle, "vid", "no");
    mpv_set_option_string(m_handle, "audio-display", "no");
    mpv_set_option_string(m_handle, "cover-art-auto", "no");
    mpv_set_option_string(m_handle, "vo", "libmpv");
    mpv_set_option_string(m_handle, "background", "none");
    mpv_set_option_string(m_handle, "gapless-audio", "yes");
    mpv_set_option_string(m_handle, "audio-stream-silence", "yes");
    mpv_set_option_string(m_handle, "audio-wait-open", "0.5");
    mpv_set_option_string(m_handle, "cache", "yes");
    mpv_set_option_string(m_handle, "cache-on-disk", "yes");
    mpv_set_option_string(m_handle, "demuxer-cache-dir", streamCacheDir().toUtf8().constData());
    mpv_set_option_string(m_handle, "demuxer-max-bytes", kDemuxerMaxBytes);
    mpv_set_option_string(m_handle, "demuxer-max-back-bytes", kDemuxerMaxBackBytes);
    mpv_set_option_string(m_handle, "stream-lavf-o", kBoundedRequests);

    const int status = mpv_initialize(m_handle);
    if (status < 0) {
        qCCritical(logPlayback) << "mpv failed to initialize" << mpv_error_string(status);
        mpv_terminate_destroy(m_handle);
        m_handle = nullptr;
        return;
    }

    mpv_observe_property(m_handle, 0, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_handle, 0, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_handle, 0, "audio-pts", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_handle, 0, "pause", MPV_FORMAT_FLAG);
    mpv_observe_property(m_handle, 0, "idle-active", MPV_FORMAT_FLAG);
    mpv_observe_property(m_handle, 0, "seeking", MPV_FORMAT_FLAG);
    mpv_observe_property(m_handle, 0, "paused-for-cache", MPV_FORMAT_FLAG);
    mpv_observe_property(m_handle, 0, "dwidth", MPV_FORMAT_INT64);
    mpv_observe_property(m_handle, 0, "dheight", MPV_FORMAT_INT64);
    mpv_request_log_messages(m_handle, "error");
    mpv_set_wakeup_callback(m_handle, &MpvController::wakeup, this);

    m_trimTimer.setInterval(kTrimIntervalMs);
    connect(&m_trimTimer, &QTimer::timeout, this, &MpvController::sampleTrim);
}

MpvController::~MpvController()
{
    if (!m_handle)
        return;

    mpv_set_wakeup_callback(m_handle, nullptr, nullptr);
    mpv_terminate_destroy(m_handle);
}

void MpvController::load(const QUrl &url, const QString &userAgent, const QString &cookie,
                         qint64 startMilliseconds, bool paused)
{
    if (!m_handle)
        return;

    setLoaded(false);
    m_hasAudioPts = false;
    m_audioPtsClock.invalidate();
    m_position = startMilliseconds;
    setFlag("pause", paused, Request::Pause);
    if (!userAgent.isEmpty())
        setProperty("user-agent", userAgent, Request::UserAgent);
    if (cookie.isEmpty())
        command({"change-list", "http-header-fields", "clr", ""}, Request::HttpHeaders);
    else
        setProperty("http-header-fields", QStringLiteral("Cookie: ") + cookie,
                    Request::HttpHeaders);
    applyFilters();
    setProperty("speed", QString::number(m_speed, 'f', 6), Request::Speed);
    setStart(startMilliseconds);
    command({"loadfile", url.toString().toUtf8(), "replace"}, Request::Load);
}

void MpvController::play()
{
    setFlag("pause", false, Request::Pause);
}

void MpvController::pause()
{
    setFlag("pause", true, Request::Pause);
}

void MpvController::stop()
{
    if (!m_handle)
        return;

    setLoaded(false);
    command({"stop"}, Request::Stop);
}

void MpvController::seek(qint64 milliseconds)
{
    if (!m_handle)
        return;

    applyFilters();
    m_seeking = true;
    Q_EMIT discontinuity();
    updatePlaying();
    command({"seek", QByteArray::number(milliseconds / 1000.0, 'f', 3), "absolute"}, Request::Seek);
}

void MpvController::setPictureUrl(const QUrl &url)
{
    if (m_pictureUrl == url)
        return;
    m_pictureUrl = url;
    syncPictureTrack();
}

void MpvController::setPictureEnabled(bool enabled)
{
    if (m_pictureEnabled == enabled)
        return;
    m_pictureEnabled = enabled;
    syncPictureTrack();
}

void MpvController::syncPictureTrack()
{
    if (!m_handle)
        return;

    if (!m_loaded) {
        if (m_pictureAttached)
            endTrimming();
        m_pictureAttached = false;
        setPictureSize(0, 0);
        return;
    }

    const bool wanted = m_pictureEnabled && !m_pictureUrl.isEmpty();
    if (wanted == m_pictureAttached)
        return;

    if (wanted) {
        command({"video-add", m_pictureUrl.toString().toUtf8(), "select"}, Request::PictureAdd);
        beginTrimming();
        qCDebug(logPlayback) << "picture attached";
    } else {
        endTrimming();
        command({"video-remove"}, Request::PictureRemove);
        qCDebug(logPlayback) << "picture detached";
        setPictureSize(0, 0);
    }
    m_pictureAttached = wanted;
}

void MpvController::beginTrimming()
{
    m_trimBox = QRect();
    m_trimSource = QSize();
    m_trimStreak = 0;
    setProperty("video-crop", QString(), Request::TrimCrop);
    setProperty("vf", kTrimFilter, Request::TrimFilter);
    m_trimTimer.start();
}

void MpvController::endTrimming()
{
    m_trimTimer.stop();
    setProperty("vf", QString(), Request::TrimFilter);
    setProperty("video-crop", QString(), Request::TrimCrop);
}

void MpvController::sampleTrim()
{
    if (m_pictureWidth <= 0 || m_pictureHeight <= 0)
        return;
    if (!m_trimSource.isValid())
        m_trimSource = QSize(m_pictureWidth, m_pictureHeight);

    readProperty("vf-metadata/trim", MPV_FORMAT_NODE, Request::TrimMetadata);
}

void MpvController::applyTrim(const QRect &box)
{
    if (!box.isValid() || !m_trimTimer.isActive())
        return;
    if (box != m_trimBox) {
        m_trimBox = box;
        m_trimStreak = 1;
        return;
    }
    if (++m_trimStreak < kTrimSamples)
        return;

    m_trimTimer.stop();
    setProperty("vf", QString(), Request::TrimFilter);
    if (!worthTrimming(box, m_trimSource))
        return;

    qCDebug(logPlayback) << "picture trimmed to" << box << "of" << m_trimSource;
    setProperty(
        "video-crop",
        QStringLiteral("%1x%2+%3+%4").arg(box.width()).arg(box.height()).arg(box.x()).arg(box.y()),
        Request::TrimCrop);
}

double MpvController::audioPts() const
{
    if (!m_hasAudioPts || !m_playing || !m_audioPtsClock.isValid())
        return m_audioPts;

    return m_audioPts + m_speed * qMin(double(m_audioPtsClock.elapsed()), kAudioPtsReachMs);
}

void MpvController::setPictureSize(int width, int height)
{
    if (m_pictureWidth == width && m_pictureHeight == height)
        return;
    m_pictureWidth = width;
    m_pictureHeight = height;
    Q_EMIT pictureSizeChanged();
}

void MpvController::setLoopFile(bool loop)
{
    setProperty("loop-file", loop ? QStringLiteral("inf") : QStringLiteral("no"),
                Request::LoopFile);
}

void MpvController::setVolume(int volume)
{
    m_volume = qBound(0, volume, 100);
    applyVolume();
}

void MpvController::setTrackGain(std::optional<double> gainDb)
{
    m_trackGainDb = gainDb;
    applyFilters();
}

void MpvController::refreshFilters()
{
    applyFilters();
}

void MpvController::setFade(const Fade &fade)
{
    if (m_fade == fade)
        return;
    m_fade = fade;
    applyFilters();
}

void MpvController::setLowShelf(double gainDb)
{
    const double bounded = qBound(-24.0, gainDb, 0.0);
    if (std::abs(m_lowShelfDb - bounded) < kRestingShelfDb)
        return;

    m_lowShelfDb = bounded;
    if (!m_loaded) {
        applyFilters();
        return;
    }

    command({"af-command", "low", "g", QByteArray::number(m_lowShelfDb, 'f', 2), "lowshelf"},
            Request::LowShelf);
}

void MpvController::setStretcher(bool enabled)
{
    if (m_stretcher == enabled)
        return;

    m_stretcher = enabled;
    applyFilters();
}

void MpvController::setTransitionFilters(bool enabled)
{
    if (m_transitionFilters == enabled)
        return;

    m_transitionFilters = enabled;
    applyFilters();
}

void MpvController::clearFadeAndShelf()
{
    if (!m_fade.active() && std::abs(m_lowShelfDb) < kRestingShelfDb)
        return;

    m_fade = {};
    m_lowShelfDb = 0.0;
    applyFilters();
}

void MpvController::setSpeed(double speed)
{
    const double bounded = qBound(0.5, speed, 2.0);
    if (std::abs(m_speed - bounded) < 0.00001)
        return;

    m_speed = bounded;
    setProperty("speed", QString::number(m_speed, 'f', 6), Request::Speed);
}

void MpvController::forgetTransition()
{
    m_fade = {};
    m_lowShelfDb = 0.0;
    m_speed = 1.0;
    m_stretcher = false;
    m_transitionFilters = false;
}

void MpvController::resetTransition()
{
    const bool loaded = m_loaded;
    forgetTransition();
    if (!loaded)
        return;

    setProperty("speed", QStringLiteral("1.000000"), Request::Speed);
    applyFilters();
}

void MpvController::setMuted(bool muted)
{
    m_muted = muted;
    setFlag("mute", muted, Request::Mute);
}

void MpvController::wakeup(void *context)
{
    auto *self = static_cast<MpvController *>(context);
    QMetaObject::invokeMethod(self, [self] { self->drainEvents(); }, Qt::QueuedConnection);
}

void MpvController::drainEvents()
{
    while (m_handle) {
        mpv_event *event = mpv_wait_event(m_handle, 0);
        if (event->event_id == MPV_EVENT_NONE)
            return;
        handleEvent(event);
    }
}

void MpvController::handleEvent(mpv_event *event)
{
    switch (event->event_id) {
    case MPV_EVENT_PROPERTY_CHANGE:
        handleProperty(static_cast<mpv_event_property *>(event->data));
        break;
    case MPV_EVENT_COMMAND_REPLY:
    case MPV_EVENT_SET_PROPERTY_REPLY:
        handleReply(Request(event->reply_userdata), event->error);
        break;
    case MPV_EVENT_GET_PROPERTY_REPLY:
        if (event->error >= 0)
            handleValue(Request(event->reply_userdata),
                        static_cast<mpv_event_property *>(event->data));
        break;
    case MPV_EVENT_LOG_MESSAGE: {
        const auto *message = static_cast<mpv_event_log_message *>(event->data);
        qCWarning(logPlayback) << "mpv" << message->prefix
                               << QString::fromUtf8(message->text).trimmed();
        break;
    }
    case MPV_EVENT_FILE_LOADED: setLoaded(true); break;
    case MPV_EVENT_PLAYBACK_RESTART:
        m_seeking = false;
        Q_EMIT discontinuity();
        updatePlaying();
        break;
    case MPV_EVENT_END_FILE: {
        const auto *end = static_cast<mpv_event_end_file *>(event->data);
        if (end->reason == MPV_END_FILE_REASON_EOF || end->reason == MPV_END_FILE_REASON_ERROR)
            Q_EMIT ending();
        setLoaded(false);
        if (end->reason == MPV_END_FILE_REASON_EOF)
            Q_EMIT trackEnded();
        else if (end->reason == MPV_END_FILE_REASON_ERROR)
            Q_EMIT failed(QString::fromUtf8(mpv_error_string(end->error)));
        break;
    }
    default: break;
    }
}

void MpvController::handleReply(Request request, int status)
{
    if (status >= 0)
        return;

    report(status, requestName(request));
    switch (request) {
    case Request::Seek:
        m_seeking = false;
        updatePlaying();
        break;
    case Request::LowShelf: applyFilters(); break;
    case Request::Filters:
        if (m_stretcher) {
            m_stretcher = false;
            applyFilters();
        }
        Q_EMIT filterChainFailed();
        break;
    default: break;
    }
}

void MpvController::handleValue(Request request, const mpv_event_property *property)
{
    if (request == Request::TrimMetadata && property->format == MPV_FORMAT_NODE)
        applyTrim(contentBox(*static_cast<mpv_node *>(property->data)));
}

void MpvController::handleProperty(mpv_event_property *property)
{
    const QLatin1String name(property->name);

    if (name == QLatin1String("dwidth") || name == QLatin1String("dheight")) {
        const auto extent =
            property->format == MPV_FORMAT_INT64 ? int(*static_cast<qint64 *>(property->data)) : 0;
        if (name == QLatin1String("dwidth"))
            setPictureSize(extent, m_pictureHeight);
        else
            setPictureSize(m_pictureWidth, extent);
        return;
    }

    if (property->format == MPV_FORMAT_DOUBLE) {
        const auto seconds = *static_cast<double *>(property->data);
        const auto milliseconds = qint64(seconds * 1000);
        if (name == QLatin1String("time-pos")) {
            m_position = milliseconds;
            Q_EMIT positionChanged(milliseconds);
        } else if (name == QLatin1String("duration")) {
            m_duration = milliseconds;
            Q_EMIT durationChanged(milliseconds);
        } else if (name == QLatin1String("audio-pts")) {
            m_audioPts = seconds * 1000.0;
            m_hasAudioPts = true;
            m_audioPtsClock.start();
        }
        return;
    }

    if (property->format == MPV_FORMAT_FLAG) {
        const bool flag = *static_cast<int *>(property->data) != 0;
        if (name == QLatin1String("pause"))
            m_paused = flag;
        else if (name == QLatin1String("idle-active"))
            m_idle = flag;
        else if (name == QLatin1String("seeking")) {
            m_seeking = flag;
            Q_EMIT discontinuity();
        } else if (name == QLatin1String("paused-for-cache"))
            m_buffering = flag;
        updatePlaying();
    }
}

void MpvController::setLoaded(bool loaded)
{
    if (m_loaded == loaded)
        return;
    m_loaded = loaded;
    syncPictureTrack();
    Q_EMIT loadedChanged(m_loaded);
    updatePlaying();
}

void MpvController::updatePlaying()
{
    const bool playing = m_loaded && !m_paused && !m_idle && !m_seeking && !m_buffering;
    if (m_playing == playing)
        return;

    m_playing = playing;
    Q_EMIT playingChanged(m_playing);
}

void MpvController::setStart(qint64 milliseconds)
{
    setProperty("start",
                milliseconds > 0 ? QString::number(milliseconds / 1000.0, 'f', 3)
                                 : QStringLiteral("none"),
                Request::Start);
}

void MpvController::applyFilters()
{
    const PlaybackSettings &settings = PlaybackSettings::instance();

    QStringList chain;
    const QString equalizer = settings.filterChain();
    if (m_transitionFilters) {
        const double gain = m_trackGainDb && settings.normalizeLoudness() ? *m_trackGainDb : 0.0;
        chain.append(QStringLiteral("@gain:lavfi=[volume=%1dB]").arg(gain, 0, 'f', 2));
        chain.append(QStringLiteral("@eq:lavfi=[%1]")
                         .arg(equalizer.isEmpty() ? QStringLiteral("anull") : equalizer));
    } else {
        if (m_trackGainDb && settings.normalizeLoudness())
            chain.append(
                QStringLiteral("@gain:lavfi=[volume=%1dB]").arg(*m_trackGainDb, 0, 'f', 2));
        if (!equalizer.isEmpty())
            chain.append(QStringLiteral("@eq:lavfi=[%1]").arg(equalizer));
    }

    chain.append(fadeFilter(m_fade));
    if (m_transitionFilters) {
        chain.append(QStringLiteral("@low:lavfi=[lowshelf=f=180:t=q:w=0.707:g=%1]")
                         .arg(m_lowShelfDb, 0, 'f', 2));
        if (m_stretcher)
            chain.append(QStringLiteral("@stretch:rubberband=window=short"));
    }
    setProperty("af", chain.join(QLatin1Char(',')), Request::Filters);
}

void MpvController::applyVolume()
{
    if (m_volume <= 0) {
        setProperty("volume", QStringLiteral("0"), Request::Volume);
        return;
    }

    setProperty("volume", QString::number(volumePercentForGain(gainDbForVolume(m_volume)), 'f', 2),
                Request::Volume);
}

void MpvController::command(const QList<QByteArray> &arguments, Request request)
{
    if (!m_handle)
        return;

    QVarLengthArray<const char *, 8> argv;
    for (const QByteArray &argument : arguments)
        argv.append(argument.constData());
    argv.append(nullptr);
    report(mpv_command_async(m_handle, quint64(request), argv.data()), requestName(request));
}

void MpvController::readProperty(const char *name, int format, Request request)
{
    if (!m_handle)
        return;

    report(mpv_get_property_async(m_handle, quint64(request), name, mpv_format(format)),
           requestName(request));
}

void MpvController::setProperty(const char *name, const QString &value, Request request)
{
    if (!m_handle)
        return;

    const QByteArray target = value.toUtf8();
    const char *data = target.constData();
    report(mpv_set_property_async(m_handle, quint64(request), name, MPV_FORMAT_STRING,
                                  static_cast<void *>(&data)),
           requestName(request));
}

void MpvController::setFlag(const char *name, bool value, Request request)
{
    if (!m_handle)
        return;

    int flag = value ? 1 : 0;
    report(mpv_set_property_async(m_handle, quint64(request), name, MPV_FORMAT_FLAG, &flag),
           requestName(request));
}

}
