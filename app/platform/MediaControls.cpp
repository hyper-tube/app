#include "MediaControls.h"

#if defined(HT_MUSIC_MPRIS)
#include "MprisService.h"
#elif defined(HT_MUSIC_SMTC)
#include "SystemMediaControls.h"
#elif defined(HT_MUSIC_NOW_PLAYING)
#include "NowPlayingControls.h"
#endif

namespace platform {

MediaControls::MediaControls(media::PlaybackController &controller, QObject *parent)
    : QObject(parent)
    , m_controller(controller)
{
}

MediaControls *MediaControls::create(media::PlaybackController &controller, QWindow *window,
                                     QObject *parent)
{
#if defined(HT_MUSIC_MPRIS)
    Q_UNUSED(window)
    return new MprisService(controller, parent);
#elif defined(HT_MUSIC_SMTC)
    return new SystemMediaControls(controller, window, parent);
#elif defined(HT_MUSIC_NOW_PLAYING)
    Q_UNUSED(window)
    return new NowPlayingControls(controller, parent);
#else
    Q_UNUSED(window)
    return new MediaControls(controller, parent);
#endif
}

void MediaControls::requestRaise()
{
    Q_EMIT raiseRequested();
}

void MediaControls::requestQuit()
{
    Q_EMIT quitRequested();
}

}
