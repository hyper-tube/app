#include "MprisPlayerAdaptor.h"

#include "MprisService.h"
#include "media/PlaybackController.h"

namespace {

constexpr qlonglong kMicrosecondsPerMillisecond = 1000;

const QString kTrackPathPrefix = QStringLiteral("/org/mpris/MediaPlayer2/htmusic/");
const QString kNoTrackPath = QStringLiteral("/org/mpris/MediaPlayer2/TrackList/NoTrack");
const QString kWatchUrl = QStringLiteral("https://music.youtube.com/watch?v=");

QDBusObjectPath trackPath(const media::Track &track)
{
    if (!track.valid())
        return QDBusObjectPath(kNoTrackPath);
    return QDBusObjectPath(kTrackPathPrefix + QString::fromLatin1(track.videoId.toUtf8().toHex()));
}

}

namespace platform {

MprisPlayerAdaptor::MprisPlayerAdaptor(MprisService *service)
    : QDBusAbstractAdaptor(service)
    , m_service(service)
{
}

media::PlaybackController &MprisPlayerAdaptor::controller() const
{
    return m_service->controller();
}

QString MprisPlayerAdaptor::playbackStatus() const
{
    if (controller().playing())
        return QStringLiteral("Playing");
    return controller().track().valid() ? QStringLiteral("Paused") : QStringLiteral("Stopped");
}

QString MprisPlayerAdaptor::loopStatus() const
{
    switch (controller().repeat()) {
    case media::PlaybackController::RepeatOne: return QStringLiteral("Track");
    case media::PlaybackController::RepeatAll: return QStringLiteral("Playlist");
    case media::PlaybackController::RepeatOff: break;
    }
    return QStringLiteral("None");
}

void MprisPlayerAdaptor::setLoopStatus(const QString &status)
{
    if (status == QLatin1String("Track"))
        controller().setRepeat(media::PlaybackController::RepeatOne);
    else if (status == QLatin1String("Playlist"))
        controller().setRepeat(media::PlaybackController::RepeatAll);
    else
        controller().setRepeat(media::PlaybackController::RepeatOff);
}

bool MprisPlayerAdaptor::shuffle() const
{
    return controller().shuffle();
}

void MprisPlayerAdaptor::setShuffle(bool shuffle)
{
    controller().setShuffle(shuffle);
}

double MprisPlayerAdaptor::volume() const
{
    return controller().muted() ? 0.0 : controller().volume() / 100.0;
}

void MprisPlayerAdaptor::setVolume(double volume)
{
    const int level = qRound(qBound(0.0, volume, 1.0) * 100);
    controller().setMuted(level == 0);
    if (level > 0)
        controller().setVolume(level);
}

QVariantMap MprisPlayerAdaptor::metadata() const
{
    const media::Track track = controller().track();

    QVariantMap fields;
    fields.insert(QStringLiteral("mpris:trackid"), QVariant::fromValue(trackPath(track)));
    if (!track.valid())
        return fields;

    fields.insert(QStringLiteral("mpris:length"),
                  qlonglong(controller().duration()) * kMicrosecondsPerMillisecond);
    fields.insert(QStringLiteral("xesam:title"), track.title);
    fields.insert(QStringLiteral("xesam:url"), kWatchUrl + track.videoId);
    if (!track.artist.isEmpty())
        fields.insert(QStringLiteral("xesam:artist"), QStringList {track.artist});
    if (!track.album.isEmpty())
        fields.insert(QStringLiteral("xesam:album"), track.album);
    if (!track.artId.isEmpty())
        fields.insert(QStringLiteral("mpris:artUrl"), track.artId);
    if (track.ratingKnown)
        fields.insert(QStringLiteral("xesam:userRating"), track.liked ? 1.0 : 0.0);
    return fields;
}

qlonglong MprisPlayerAdaptor::position() const
{
    return qlonglong(controller().position()) * kMicrosecondsPerMillisecond;
}

bool MprisPlayerAdaptor::canGoNext() const
{
    return controller().canGoNext();
}

bool MprisPlayerAdaptor::canGoPrevious() const
{
    return controller().canGoPrevious();
}

bool MprisPlayerAdaptor::canPlay() const
{
    return controller().track().valid();
}

bool MprisPlayerAdaptor::canPause() const
{
    return controller().track().valid();
}

bool MprisPlayerAdaptor::canSeek() const
{
    return controller().duration() > 0;
}

void MprisPlayerAdaptor::reportSeek(qlonglong microseconds)
{
    Q_EMIT Seeked(microseconds);
}

void MprisPlayerAdaptor::Next()
{
    controller().next();
}

void MprisPlayerAdaptor::Previous()
{
    controller().previous();
}

void MprisPlayerAdaptor::Pause()
{
    controller().pause();
}

void MprisPlayerAdaptor::PlayPause()
{
    controller().toggle();
}

void MprisPlayerAdaptor::Stop()
{
    controller().pause();
    controller().seek(0);
}

void MprisPlayerAdaptor::Play()
{
    controller().play();
}

void MprisPlayerAdaptor::Seek(qlonglong offset)
{
    controller().seek(controller().position() + offset / kMicrosecondsPerMillisecond);
}

void MprisPlayerAdaptor::SetPosition(const QDBusObjectPath &track, qlonglong position)
{
    if (track != trackPath(controller().track()))
        return;
    controller().seek(position / kMicrosecondsPerMillisecond);
}

void MprisPlayerAdaptor::OpenUri(const QString &uri)
{
    controller().playVideoIds(uri);
}

}
