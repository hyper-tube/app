#pragma once

#include "MediaControls.h"

#include <memory>

class QWindow;

namespace platform {

class SystemMediaControls : public MediaControls
{
    Q_OBJECT

public:
    SystemMediaControls(media::PlaybackController &controller, QWindow *window, QObject *parent);
    ~SystemMediaControls() override;

private:
    class Session;

    void publishStatus();
    void publishTrack();
    void publishReach();
    void publishTimeline();

    std::unique_ptr<Session> m_session;
};

}
