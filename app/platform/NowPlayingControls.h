#pragma once

#include "MediaControls.h"

#include <QString>

#include <memory>

namespace platform {

class NowPlayingControls : public MediaControls
{
    Q_OBJECT

public:
    NowPlayingControls(media::PlaybackController &controller, QObject *parent);
    ~NowPlayingControls() override;

private:
    class Session;

    void bindCommands();
    void publishStatus();
    void publishTrack();
    void publishTimeline();
    void publishArtwork(const QString &source);
    void commit();

    std::unique_ptr<Session> m_session;
};

}
