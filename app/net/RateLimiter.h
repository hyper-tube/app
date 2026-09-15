#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QQueue>
#include <QTimer>

#include <functional>

namespace net {

class RateLimiter : public QObject
{
    Q_OBJECT

public:
    using Task = std::function<void()>;

    explicit RateLimiter(int minimumIntervalMs, QObject *parent = nullptr);

    void schedule(Task task, bool background = false);

private:
    void drain();

    QTimer m_timer;
    QQueue<Task> m_queue;
    QQueue<Task> m_background;
    QElapsedTimer m_sinceLastDispatch;
    int m_minimumIntervalMs;
};

}
