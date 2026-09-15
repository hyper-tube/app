#include "DiscordPresence.h"

#include "DiscordIpc.h"
#include "control/ControlCenter.h"
#include "core/Logging.h"
#include "media/PlaybackController.h"
#include "plugin/PluginRegistry.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QJsonArray>
#include <QTimer>
#include <QUuid>

namespace {

const QString kId = QStringLiteral("discord");
const QString kApplicationId = QStringLiteral("1546986058498056294");
const QString kFailureId = QStringLiteral("plugin.discord.failed");

const QString kSharingKey = QStringLiteral("sharing");
const QString kWhilePausedKey = QStringLiteral("whilePaused");
const QString kShowArtworkKey = QStringLiteral("showArtwork");
const QString kShowElapsedKey = QStringLiteral("showElapsed");
const QString kViewSongKey = QStringLiteral("viewSong");
const QString kInstallAppKey = QStringLiteral("installApp");
const QString kActivityKey = QStringLiteral("activity");

constexpr qint64 kMinimumInterval = 5000;
constexpr int kFirstRetry = 2000;
constexpr int kMaximumRetry = 60000;
constexpr int kReassertInterval = 15000;
constexpr int kShortestField = 2;
constexpr int kLongestField = 128;
constexpr int kLongestLabel = 32;

const QString kPlayingAsset = QStringLiteral("playing");
const QString kPausedAsset = QStringLiteral("paused");
const QString kDownloadUrl = QStringLiteral("https://htm.nekolab.app/download");
const QString kThumbnailHost = QStringLiteral("i.ytimg.com/vi/");
const QString kLetterboxed = QStringLiteral("/hqdefault.jpg");
const QString kUnbarred = QStringLiteral("/mqdefault.jpg");

QString framed(const QString &url)
{
    if (!url.contains(kThumbnailHost))
        return url;
    const qsizetype query = url.indexOf(QLatin1Char('?'));
    QString sized = query < 0 ? url : url.first(query);
    if (sized.endsWith(kLetterboxed))
        sized.replace(kLetterboxed, kUnbarred);
    return sized;
}

QString clamped(const QString &text)
{
    if (text.size() > kLongestField)
        return text.left(kLongestField);
    if (text.size() < kShortestField)
        return text + QString(kShortestField - text.size(), QLatin1Char(' '));
    return text;
}

}

namespace plugin::discord {

DiscordPresence::DiscordPresence(QObject *parent)
    : Plugin(parent)
    , m_ipc(new DiscordIpc(this))
    , m_throttle(new QTimer(this))
    , m_retry(new QTimer(this))
    , m_reassert(new QTimer(this))
{
    m_throttle->setSingleShot(true);
    m_retry->setSingleShot(true);
    m_reassert->setInterval(kReassertInterval);

    connect(m_throttle, &QTimer::timeout, this, &DiscordPresence::publishNow);
    connect(m_retry, &QTimer::timeout, this, &DiscordPresence::refresh);
    connect(m_reassert, &QTimer::timeout, this, [this] {
        if (m_sent.isValid() && m_sent.elapsed() < kMinimumInterval)
            return;
        publish(true);
    });

    connect(m_ipc, &DiscordIpc::ready, this, [this] {
        qCInfo(logPlugins) << "discord presence connected";
        m_backoff = 0;
        m_published = {};
        control::ControlCenter::instance().retract(kFailureId);
        m_reassert->start();
        publishNow();
        restate();
    });
    connect(m_ipc, &DiscordIpc::closed, this, [this] {
        m_published = {};
        m_reassert->stop();
        restate();
        if (enabled() && sharing() && !m_refused)
            retryLater();
    });
    connect(m_ipc, &DiscordIpc::rejected, this, &DiscordPresence::noteFailure);

    media::PlaybackController const &playback = media::PlaybackController::instance();
    connect(&playback, &media::PlaybackController::trackChanged, this, &DiscordPresence::schedule);
    connect(&playback, &media::PlaybackController::playingChanged, this,
            &DiscordPresence::schedule);
    connect(&playback, &media::PlaybackController::durationChanged, this,
            &DiscordPresence::schedule);
    connect(&playback, &media::PlaybackController::seeked, this, &DiscordPresence::schedule);

    connect(&control::ControlCenter::instance(), &control::ControlCenter::actionInvoked, this,
            [](const QString &id) {
        if (id == kFailureId)
            PluginRegistry::instance().openPage(kId);
    });
}

DiscordPresence &DiscordPresence::instance()
{
    static auto *presence = new DiscordPresence(QCoreApplication::instance());
    return *presence;
}

DiscordPresence *DiscordPresence::create(QQmlEngine *, QJSEngine *)
{
    QQmlEngine::setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
    return &instance();
}

PluginInfo DiscordPresence::info() const
{
    PluginInfo entry;
    entry.id = kId;
    entry.name = tr("Discord Rich Presence");
    entry.description = tr("Show what you are playing on your Discord profile.");
    entry.about = tr("Publishes the song you are listening to over Discord's local socket, so "
                     "your profile carries the title, the artist, the cover art and how far "
                     "into the track you are.\n\n"
                     "Discord has to be running on this machine. The connection is a socket on "
                     "this computer rather than a web request, and nothing is sent anywhere "
                     "else.");
    entry.icon = QStringLiteral("extension");
    entry.iconSource = assetSource(kId, QStringLiteral("discord.svg"));
    entry.cardSource = qmlSource(kId, QStringLiteral("DiscordCard"));
    return entry;
}

QList<PluginSetting> DiscordPresence::schema() const
{
    PluginSetting sharing;
    sharing.key = kSharingKey;
    sharing.kind = PluginSetting::Toggle;
    sharing.label = tr("Share what you are listening to");
    sharing.caption = tr("Turning this off clears your status and closes the connection");
    sharing.fallback = true;

    PluginSetting whilePaused;
    whilePaused.key = kWhilePausedKey;
    whilePaused.kind = PluginSetting::Toggle;
    whilePaused.label = tr("Keep the status while paused");
    whilePaused.caption = tr("The progress bar is dropped, because a paused track does not move");
    whilePaused.fallback = false;

    PluginSetting artwork;
    artwork.key = kShowArtworkKey;
    artwork.kind = PluginSetting::Toggle;
    artwork.label = tr("Show the cover art");
    artwork.fallback = true;

    PluginSetting elapsed;
    elapsed.key = kShowElapsedKey;
    elapsed.kind = PluginSetting::Toggle;
    elapsed.label = tr("Show the elapsed time");
    elapsed.caption = tr("Draws a progress bar under the track on your profile");
    elapsed.fallback = true;

    PluginSetting viewSong;
    viewSong.key = kViewSongKey;
    viewSong.kind = PluginSetting::Toggle;
    viewSong.label = tr("Show a View song button");
    viewSong.caption = tr("Opens the track on YouTube Music. Never shown for your own uploads, "
                          "because the link goes nowhere");
    viewSong.fallback = true;

    PluginSetting installApp;
    installApp.key = kInstallAppKey;
    installApp.kind = PluginSetting::Toggle;
    installApp.label = tr("Show an Install HyperTube button");
    installApp.caption = tr("Points at the download page, so anyone reading your status can get "
                            "the same client");
    installApp.fallback = true;

    PluginSetting activity;
    activity.key = kActivityKey;
    activity.kind = PluginSetting::Select;
    activity.label = tr("How Discord describes it");
    activity.fallback = QStringLiteral("listening");
    activity.options = {
        selectOption(QStringLiteral("listening"), tr("Listening to")),
        selectOption(QStringLiteral("playing"), tr("Playing")),
    };

    return {sharing, whilePaused, artwork, elapsed, viewSong, installApp, activity};
}

bool DiscordPresence::sharing() const
{
    return value(kSharingKey).toBool();
}

bool DiscordPresence::connected() const
{
    return m_ipc->connected();
}

void DiscordPresence::setSharing(bool sharing)
{
    setValue(kSharingKey, sharing);
}

media::Track DiscordPresence::shown() const
{
    if (!enabled() || !sharing() || !m_ipc->connected())
        return {};
    const media::PlaybackController &playback = media::PlaybackController::instance();
    const media::Track track = playback.track();
    if (!track.valid())
        return {};
    if (!playback.playing() && !value(kWhilePausedKey).toBool())
        return {};
    return track;
}

void DiscordPresence::start()
{
    m_refused = false;
    m_backoff = 0;
    m_failure.clear();
    refresh();
}

void DiscordPresence::stop()
{
    m_throttle->stop();
    m_retry->stop();
    m_reassert->stop();
    m_published = {};
    m_ipc->close();
    control::ControlCenter::instance().retract(kFailureId);
    setState(Ok, {});
    Q_EMIT presenceChanged();
}

void DiscordPresence::valueChanged(const QString &key)
{
    if (key == kSharingKey) {
        m_backoff = 0;
        m_retry->stop();
        refresh();
        return;
    }
    m_published = {};
    schedule();
    restate();
}

void DiscordPresence::reconnect()
{
    m_refused = false;
    m_failure.clear();
    m_backoff = 0;
    m_retry->stop();
    control::ControlCenter::instance().retract(kFailureId);
    m_ipc->close();
    refresh();
}

void DiscordPresence::refresh()
{
    if (!enabled() || !sharing() || m_refused) {
        m_throttle->stop();
        m_retry->stop();
        m_reassert->stop();
        m_ipc->close();
        restate();
        return;
    }
    if (m_ipc->connected())
        schedule();
    else if (!m_ipc->connecting())
        m_ipc->open(kApplicationId);
    restate();
}

void DiscordPresence::schedule()
{
    if (!enabled() || !m_ipc->connected())
        return;
    const qint64 since = m_sent.isValid() ? m_sent.elapsed() : kMinimumInterval;
    if (since >= kMinimumInterval)
        publishNow();
    else if (!m_throttle->isActive())
        m_throttle->start(int(kMinimumInterval - since));
}

void DiscordPresence::publishNow()
{
    publish(false);
}

void DiscordPresence::publish(bool force)
{
    if (!m_ipc->connected())
        return;
    m_throttle->stop();
    const QJsonObject current = activity();
    const bool changed = current != m_published || !m_sent.isValid();
    if (!changed && !force)
        return;
    if (changed)
        qCDebug(logPlugins) << "discord activity"
                            << (current.isEmpty() ? QString() : shown().title);
    m_published = current;
    m_sent.restart();

    QJsonObject arguments {{QStringLiteral("pid"), QCoreApplication::applicationPid()}};
    arguments.insert(QStringLiteral("activity"),
                     current.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(current));
    m_ipc->send({{QStringLiteral("cmd"), QStringLiteral("SET_ACTIVITY")},
                 {QStringLiteral("nonce"), QUuid::createUuid().toString(QUuid::WithoutBraces)},
                 {QStringLiteral("args"), arguments}});
    restate();
}

void DiscordPresence::retryLater()
{
    m_backoff = m_backoff == 0 ? kFirstRetry : qMin(m_backoff * 2, kMaximumRetry);
    m_retry->start(m_backoff);
}

QJsonObject DiscordPresence::activity() const
{
    const media::Track track = shown();
    if (!track.valid())
        return {};

    const media::PlaybackController &playback = media::PlaybackController::instance();
    QJsonObject payload;
    payload.insert(QStringLiteral("type"),
                   value(kActivityKey).toString() == QLatin1String("playing") ? 0 : 2);
    payload.insert(QStringLiteral("details"), clamped(track.title));
    payload.insert(QStringLiteral("state"),
                   clamped(track.artist.isEmpty() ? tr("Unknown artist") : track.artist));

    if (playback.playing() && value(kShowElapsedKey).toBool()) {
        const qint64 start = QDateTime::currentMSecsSinceEpoch() - playback.position();
        QJsonObject timestamps {{QStringLiteral("start"), start}};
        if (playback.duration() > 0)
            timestamps.insert(QStringLiteral("end"), start + playback.duration());
        payload.insert(QStringLiteral("timestamps"), timestamps);
    }

    if (value(kShowArtworkKey).toBool() && track.artId.startsWith(QLatin1String("https://"))) {
        QJsonObject assets {{QStringLiteral("large_image"), framed(track.artId)}};
        if (!track.album.isEmpty())
            assets.insert(QStringLiteral("large_text"), clamped(track.album));
        assets.insert(QStringLiteral("small_image"),
                      playback.playing() ? kPlayingAsset : kPausedAsset);
        assets.insert(QStringLiteral("small_text"),
                      clamped(playback.playing() ? tr("Playing") : tr("Paused")));
        payload.insert(QStringLiteral("assets"), assets);
    }

    QJsonArray buttons;
    if (value(kViewSongKey).toBool() && !track.upload) {
        buttons.append(
            QJsonObject {{QStringLiteral("label"), tr("View song").left(kLongestLabel)},
                         {QStringLiteral("url"),
                          QStringLiteral("https://music.youtube.com/watch?v=") + track.videoId}});
    }
    if (value(kInstallAppKey).toBool()) {
        buttons.append(
            QJsonObject {{QStringLiteral("label"), tr("Install HyperTube").left(kLongestLabel)},
                         {QStringLiteral("url"), kDownloadUrl}});
    }
    if (!buttons.isEmpty())
        payload.insert(QStringLiteral("buttons"), buttons);

    return payload;
}

void DiscordPresence::restate()
{
    if (!enabled()) {
        setState(Ok, {});
    } else if (m_refused) {
        setState(Error, m_failure.isEmpty() ? tr("Discord refused the connection.") : m_failure);
    } else if (!sharing()) {
        setState(Ok, tr("Paused. Nothing is shared right now."));
    } else if (m_ipc->connected()) {
        setState(Ok,
                 shown().valid() ? tr("Connected to Discord.")
                                 : tr("Connected. Nothing is playing."));
    } else if (m_ipc->connecting()) {
        setState(Busy, tr("Looking for Discord..."));
    } else {
        setState(Warning, tr("Discord is not running. Reconnecting automatically."));
    }
    Q_EMIT presenceChanged();
}

void DiscordPresence::noteFailure(const QString &message)
{
    m_refused = true;
    m_failure = message;
    m_retry->stop();
    qCWarning(logPlugins) << "discord presence refused" << message;

    control::Notification entry;
    entry.id = kFailureId;
    entry.icon = QStringLiteral("extension_off");
    entry.title = tr("Discord presence stopped");
    entry.body = message.isEmpty() ? tr("Discord refused the connection.")
                                   : tr("Discord refused the connection: %1").arg(message);
    entry.action = tr("Open settings");
    control::ControlCenter::instance().publish(entry);
    restate();
}

}
