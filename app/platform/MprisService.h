#pragma once

#include "MediaControls.h"

#include <QStringList>
#include <QTimer>

namespace platform {

class MprisPlayerAdaptor;

class MprisService : public MediaControls
{
    Q_OBJECT

public:
    MprisService(media::PlaybackController &controller, QObject *parent);

private:
    void publish(const QStringList &properties);
    void flush();

    MprisPlayerAdaptor *m_player;
    QTimer m_flushTimer;
    QStringList m_dirty;
};

}
