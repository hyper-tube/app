#include "SystemMediaControls.h"

#include "core/Logging.h"
#include "media/PlaybackController.h"

#include <QUrl>
#include <QWindow>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.h>
#include <winrt/Windows.Storage.Streams.h>

#include <systemmediatransportcontrolsinterop.h>

#include <chrono>

using winrt::Windows::Foundation::TimeSpan;
using winrt::Windows::Foundation::Uri;
using winrt::Windows::Media::MediaPlaybackStatus;
using winrt::Windows::Media::MediaPlaybackType;
using winrt::Windows::Media::SystemMediaTransportControls;
using winrt::Windows::Media::SystemMediaTransportControlsButton;
using winrt::Windows::Media::SystemMediaTransportControlsTimelineProperties;
using winrt::Windows::Storage::Streams::RandomAccessStreamReference;

namespace {

SystemMediaTransportControls controlsFor(QWindow *window)
{
    if (!window)
        return nullptr;

    const HWND handle = reinterpret_cast<HWND>(window->winId());
    if (!handle)
        return nullptr;

    auto interop = winrt::get_activation_factory<SystemMediaTransportControls,
                                                 ISystemMediaTransportControlsInterop>();
    SystemMediaTransportControls controls {nullptr};
    if (FAILED(interop->GetForWindow(handle, winrt::guid_of<SystemMediaTransportControls>(),
                                     winrt::put_abi(controls)))) {
        return nullptr;
    }
    return controls;
}

MediaPlaybackStatus statusOf(const media::PlaybackController &controller)
{
    if (controller.playing())
        return MediaPlaybackStatus::Playing;
    return controller.track().valid() ? MediaPlaybackStatus::Paused : MediaPlaybackStatus::Stopped;
}

}

namespace platform {

class SystemMediaControls::Session
{
public:
    SystemMediaTransportControls controls {nullptr};
    winrt::event_token buttonPressed {};
    winrt::event_token positionRequested {};
};

SystemMediaControls::SystemMediaControls(media::PlaybackController &controller, QWindow *window,
                                         QObject *parent)
    : MediaControls(controller, parent)
    , m_session(std::make_unique<Session>())
{
    m_session->controls = controlsFor(window);
    if (!m_session->controls) {
        qCWarning(logPlatform) << "no transport controls, system media keys are unavailable";
        return;
    }

    m_session->controls.IsEnabled(true);
    m_session->controls.IsPlayEnabled(true);
    m_session->controls.IsPauseEnabled(true);
    m_session->controls.IsStopEnabled(true);

    m_session->buttonPressed = m_session->controls.ButtonPressed(
        [this](const SystemMediaTransportControls &, const auto &args) {
        const SystemMediaTransportControlsButton button = args.Button();
        QMetaObject::invokeMethod(this, [this, button] {
            media::PlaybackController &player = MediaControls::controller();
            switch (button) {
            case SystemMediaTransportControlsButton::Play: player.play(); break;
            case SystemMediaTransportControlsButton::Pause:
            case SystemMediaTransportControlsButton::Stop: player.pause(); break;
            case SystemMediaTransportControlsButton::Next: player.next(); break;
            case SystemMediaTransportControlsButton::Previous: player.previous(); break;
            default: break;
            }
        }, Qt::QueuedConnection);
    });

    m_session->positionRequested = m_session->controls.PlaybackPositionChangeRequested(
        [this](const SystemMediaTransportControls &, const auto &args) {
        const TimeSpan requested = args.RequestedPlaybackPosition();
        const qint64 milliseconds =
            std::chrono::duration_cast<std::chrono::milliseconds>(requested).count();
        QMetaObject::invokeMethod(this, [this, milliseconds] {
            MediaControls::controller().seek(milliseconds);
        }, Qt::QueuedConnection);
    });

    using Controller = media::PlaybackController;
    connect(&controller, &Controller::playingChanged, this, &SystemMediaControls::publishStatus);
    connect(&controller, &Controller::trackChanged, this, &SystemMediaControls::publishTrack);
    connect(&controller, &Controller::reachChanged, this, &SystemMediaControls::publishReach);
    connect(&controller, &Controller::durationChanged, this, &SystemMediaControls::publishTimeline);
    connect(&controller, &Controller::seeked, this, &SystemMediaControls::publishTimeline);

    publishStatus();
    publishTrack();
    publishReach();

    qCInfo(logPlatform) << "media controls published to the system transport controls";
}

SystemMediaControls::~SystemMediaControls()
{
    if (!m_session->controls)
        return;

    m_session->controls.ButtonPressed(m_session->buttonPressed);
    m_session->controls.PlaybackPositionChangeRequested(m_session->positionRequested);
    m_session->controls.IsEnabled(false);
}

void SystemMediaControls::publishStatus()
{
    if (!m_session->controls)
        return;
    m_session->controls.PlaybackStatus(statusOf(controller()));
}

void SystemMediaControls::publishTrack()
{
    if (!m_session->controls)
        return;

    const media::Track track = controller().track();
    auto updater = m_session->controls.DisplayUpdater();

    updater.Type(MediaPlaybackType::Music);
    updater.MusicProperties().Title(track.title.toStdWString());
    updater.MusicProperties().Artist(track.artist.toStdWString());
    updater.MusicProperties().AlbumTitle(track.album.toStdWString());

    const QUrl artwork(track.artId);
    if (artwork.isValid() && !artwork.isEmpty())
        updater.Thumbnail(
            RandomAccessStreamReference::CreateFromUri(Uri(artwork.toString().toStdWString())));
    else
        updater.Thumbnail(nullptr);

    updater.Update();
    publishTimeline();
}

void SystemMediaControls::publishReach()
{
    if (!m_session->controls)
        return;
    m_session->controls.IsNextEnabled(controller().canGoNext());
    m_session->controls.IsPreviousEnabled(controller().canGoPrevious());
}

void SystemMediaControls::publishTimeline()
{
    if (!m_session->controls)
        return;

    const std::chrono::milliseconds position(controller().position());
    const std::chrono::milliseconds duration(controller().duration());

    SystemMediaTransportControlsTimelineProperties timeline;
    timeline.StartTime(TimeSpan::zero());
    timeline.MinSeekTime(TimeSpan::zero());
    timeline.Position(position);
    timeline.MaxSeekTime(duration);
    timeline.EndTime(duration);
    m_session->controls.UpdateTimelineProperties(timeline);
}

}
