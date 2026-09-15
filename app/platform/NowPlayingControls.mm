#include "NowPlayingControls.h"

#include "core/Logging.h"
#include "media/PlaybackController.h"

#include <QPointer>
#include <QUrl>

#import <AppKit/AppKit.h>
#import <MediaPlayer/MediaPlayer.h>

namespace {

constexpr double kMillisecondsPerSecond = 1000.0;

NSImage *imageFrom(const QUrl &source, NSData *data)
{
    if (source.isLocalFile())
        return [[NSImage alloc] initWithContentsOfFile:source.toLocalFile().toNSString()];
    return data ? [[NSImage alloc] initWithData:data] : nil;
}

}

namespace platform {

class NowPlayingControls::Session
{
public:
    NSMutableDictionary *info = [NSMutableDictionary dictionary];
    NSMutableArray *handlers = [NSMutableArray array];
    NSURLSessionDataTask *artworkTask = nil;
    QString artworkSource;
};

NowPlayingControls::NowPlayingControls(media::PlaybackController &controller, QObject *parent)
    : MediaControls(controller, parent)
    , m_session(std::make_unique<Session>())
{
    bindCommands();

    using Controller = media::PlaybackController;
    connect(&controller, &Controller::playingChanged, this, &NowPlayingControls::publishStatus);
    connect(&controller, &Controller::trackChanged, this, &NowPlayingControls::publishTrack);
    connect(&controller, &Controller::durationChanged, this, &NowPlayingControls::publishTimeline);
    connect(&controller, &Controller::seeked, this, &NowPlayingControls::publishTimeline);

    publishTrack();
    publishStatus();

    qCInfo(logPlatform) << "media controls published to the now playing center";
}

NowPlayingControls::~NowPlayingControls()
{
    [m_session->artworkTask cancel];

    for (NSArray *entry in m_session->handlers)
        [(MPRemoteCommand *)entry[0] removeTarget:entry[1]];

    [MPNowPlayingInfoCenter defaultCenter].nowPlayingInfo = nil;
    [MPNowPlayingInfoCenter defaultCenter].playbackState = MPNowPlayingPlaybackStateStopped;
}

void NowPlayingControls::bindCommands()
{
    MPRemoteCommandCenter *center = [MPRemoteCommandCenter sharedCommandCenter];
    QPointer<NowPlayingControls> guard(this);

    using Action = void (media::PlaybackController::*)();
    auto bind = [this, guard](MPRemoteCommand *command, Action action) {
        command.enabled = YES;
        id target = [command addTargetWithHandler:^(MPRemoteCommandEvent *) {
            if (!guard)
                return MPRemoteCommandHandlerStatusCommandFailed;
            QMetaObject::invokeMethod(guard, [guard, action] { (guard->controller().*action)(); },
                                      Qt::QueuedConnection);
            return MPRemoteCommandHandlerStatusSuccess;
        }];
        [m_session->handlers addObject:@[ command, target ]];
    };

    bind(center.playCommand, &media::PlaybackController::play);
    bind(center.pauseCommand, &media::PlaybackController::pause);
    bind(center.stopCommand, &media::PlaybackController::pause);
    bind(center.togglePlayPauseCommand, &media::PlaybackController::toggle);
    bind(center.nextTrackCommand, &media::PlaybackController::next);
    bind(center.previousTrackCommand, &media::PlaybackController::previous);

    center.changePlaybackPositionCommand.enabled = YES;
    id seekTarget =
        [center.changePlaybackPositionCommand addTargetWithHandler:^(MPRemoteCommandEvent *event) {
            if (!guard)
                return MPRemoteCommandHandlerStatusCommandFailed;
            const auto *positionEvent = (MPChangePlaybackPositionCommandEvent *)event;
            const qint64 milliseconds = positionEvent.positionTime * kMillisecondsPerSecond;
            QMetaObject::invokeMethod(guard, [guard, milliseconds] {
                guard->controller().seek(milliseconds);
            }, Qt::QueuedConnection);
            return MPRemoteCommandHandlerStatusSuccess;
        }];
    [m_session->handlers addObject:@[ center.changePlaybackPositionCommand, seekTarget ]];
}

void NowPlayingControls::publishStatus()
{
    MPNowPlayingInfoCenter *center = [MPNowPlayingInfoCenter defaultCenter];
    if (controller().playing())
        center.playbackState = MPNowPlayingPlaybackStatePlaying;
    else
        center.playbackState = controller().track().valid() ? MPNowPlayingPlaybackStatePaused
                                                            : MPNowPlayingPlaybackStateStopped;
    publishTimeline();
}

void NowPlayingControls::publishTrack()
{
    const media::Track track = controller().track();

    m_session->info[MPMediaItemPropertyTitle] = track.title.toNSString();
    m_session->info[MPMediaItemPropertyArtist] = track.artist.toNSString();
    m_session->info[MPMediaItemPropertyAlbumTitle] = track.album.toNSString();
    m_session->info[MPNowPlayingInfoPropertyMediaType] = @(MPNowPlayingInfoMediaTypeAudio);

    publishArtwork(track.artId);
    publishTimeline();
}

void NowPlayingControls::publishTimeline()
{
    m_session->info[MPMediaItemPropertyPlaybackDuration] =
        @(controller().duration() / kMillisecondsPerSecond);
    m_session->info[MPNowPlayingInfoPropertyElapsedPlaybackTime] =
        @(controller().position() / kMillisecondsPerSecond);
    m_session->info[MPNowPlayingInfoPropertyPlaybackRate] = @(controller().playing() ? 1.0 : 0.0);
    commit();
}

void NowPlayingControls::publishArtwork(const QString &source)
{
    if (source == m_session->artworkSource)
        return;

    m_session->artworkSource = source;
    [m_session->artworkTask cancel];
    m_session->artworkTask = nil;
    [m_session->info removeObjectForKey:MPMediaItemPropertyArtwork];

    const QUrl artwork(source);
    if (source.isEmpty() || !artwork.isValid())
        return;

    QPointer<NowPlayingControls> guard(this);
    auto adopt = [guard, source, artwork](NSData *data) {
        NSImage *image = imageFrom(artwork, data);
        if (!image)
            return;
        dispatch_async(dispatch_get_main_queue(), ^{
            if (!guard || guard->m_session->artworkSource != source)
                return;
            guard->m_session->info[MPMediaItemPropertyArtwork] =
                [[MPMediaItemArtwork alloc] initWithBoundsSize:image.size
                                                requestHandler:^(CGSize) { return image; }];
            guard->commit();
        });
    };

    if (artwork.isLocalFile()) {
        adopt(nil);
        return;
    }

    NSURL *url = [NSURL URLWithString:artwork.toString().toNSString()];
    if (!url)
        return;

    m_session->artworkTask = [[NSURLSession sharedSession]
          dataTaskWithURL:url
        completionHandler:^(NSData *data, NSURLResponse *, NSError *error) {
            if (!error && data)
                adopt(data);
        }];
    [m_session->artworkTask resume];
}

void NowPlayingControls::commit()
{
    [MPNowPlayingInfoCenter defaultCenter].nowPlayingInfo = m_session->info;
}

}
