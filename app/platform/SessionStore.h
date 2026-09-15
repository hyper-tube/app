#pragma once

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QTimer>

namespace media {
class PlaybackController;
}

namespace platform {

class SessionStore : public QObject
{
    Q_OBJECT

public:
    explicit SessionStore(media::PlaybackController &controller, QObject *parent = nullptr);
    ~SessionStore() override;

    void restore();
    QDateTime playedAt() const { return m_playedAt.isValid() ? m_playedAt : m_savedAt; }

private:
    void save();

    media::PlaybackController &m_controller;
    QTimer m_saveTimer;
    QDateTime m_playedAt;
    QDateTime m_savedAt;
    QString m_path;
};

}
