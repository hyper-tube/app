#include "RateLimiter.h"

namespace net {

RateLimiter::RateLimiter(int minimumIntervalMs, QObject *parent)
    : QObject(parent)
    , m_minimumIntervalMs(minimumIntervalMs)
{
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &RateLimiter::drain);
}

void RateLimiter::schedule(Task task, bool background)
{
    (background ? m_background : m_queue).enqueue(std::move(task));
    if (!m_timer.isActive())
        drain();
}

void RateLimiter::drain()
{
    if (m_queue.isEmpty() && m_background.isEmpty())
        return;

    const qint64 waited =
        m_sinceLastDispatch.isValid() ? m_sinceLastDispatch.elapsed() : m_minimumIntervalMs;
    if (waited < m_minimumIntervalMs) {
        m_timer.start(m_minimumIntervalMs - int(waited));
        return;
    }

    m_sinceLastDispatch.restart();
    const Task task = m_queue.isEmpty() ? m_background.dequeue() : m_queue.dequeue();
    task();

    if (!m_queue.isEmpty() || !m_background.isEmpty())
        m_timer.start(m_minimumIntervalMs);
}

}
