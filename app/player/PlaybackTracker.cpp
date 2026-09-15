#include "PlaybackTracker.h"

#include "core/Logging.h"

#include <QRandomGenerator>
#include <QUrlQuery>

namespace {

constexpr qint64 kFallbackIntervalMs = 40000;
constexpr qint64 kDiscontinuityToleranceMs = 1500;
const QString kNonceAlphabet =
    QStringLiteral("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_");

QString nonce()
{
    QString value;
    for (int i = 0; i < 16; ++i)
        value.append(kNonceAlphabet.at(QRandomGenerator::system()->bounded(64)));
    return value;
}

QString seconds(qint64 milliseconds)
{
    return QString::number(milliseconds / 1000.0, 'f', 3);
}

}

namespace player {

PlaybackTracker::PlaybackTracker(QObject *parent)
    : QObject(parent)
{
    m_timer.setSingleShot(true);
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, [this] {
        flush(false);
        schedule();
    });
}

void PlaybackTracker::prepare(const PlaybackTrackingSeed &seed)
{
    finish();
    m_seed = seed;
}

void PlaybackTracker::setSeed(const PlaybackTrackingSeed &seed)
{
    if (!m_nonce.isEmpty())
        return;
    m_seed = seed;
    m_anchored = false;
}

void PlaybackTracker::setPlaying(bool playing)
{
    if (m_playing == playing)
        return;
    if (!playing)
        flush(false);
    m_playing = playing;
    m_anchored = false;
}

void PlaybackTracker::observe(qint64 positionMs)
{
    if (positionMs < 0)
        return;
    if (m_seed.durationMs > 0)
        positionMs = qMin(positionMs, m_seed.durationMs);
    const qint64 elapsed = m_observationClock.isValid() ? m_observationClock.restart() : 0;
    if (!m_observationClock.isValid())
        m_observationClock.start();
    if (!m_playing || !m_anchored) {
        m_anchor = positionMs;
        m_position = positionMs;
        m_anchored = m_playing;
        return;
    }
    const qint64 advance = positionMs - m_position;
    if (advance < 0 || advance > elapsed + kDiscontinuityToleranceMs) {
        flush(false);
        m_anchor = positionMs;
        m_position = positionMs;
        return;
    }
    if (advance > 0 && m_nonce.isEmpty() && !m_seed.playbackUrl.isEmpty())
        start();
    m_position = positionMs;
}

void PlaybackTracker::discontinuity()
{
    flush(false);
    m_anchored = false;
}

void PlaybackTracker::finish()
{
    flush(true);
    cancel();
}

void PlaybackTracker::cancel()
{
    m_timer.stop();
    m_nonce.clear();
    m_seed = {};
    m_playing = false;
    m_anchored = false;
    m_observationClock.invalidate();
}

QUrl PlaybackTracker::requestUrl(const QUrl &base) const
{
    QUrl url(base);
    QUrlQuery query(url);
    for (const QString &key :
         {QStringLiteral("c"), QStringLiteral("cpn"), QStringLiteral("ver"), QStringLiteral("cmt"),
          QStringLiteral("st"), QStringLiteral("et"), QStringLiteral("final")})
        query.removeAllQueryItems(key);
    query.addQueryItem(QStringLiteral("c"), m_seed.clientName);
    query.addQueryItem(QStringLiteral("cpn"), m_nonce);
    query.addQueryItem(QStringLiteral("ver"), QStringLiteral("2"));
    query.addQueryItem(QStringLiteral("cmt"), seconds(m_position));
    url.setQuery(query);
    return url;
}

void PlaybackTracker::start()
{
    m_nonce = nonce();
    m_wallClock.start();
    qCDebug(logPlayback) << "tracking session started";
    Q_EMIT pingRequested(m_seed.clientKey, requestUrl(m_seed.playbackUrl));
    if (m_seed.defaultFlushMs <= 0) {
        m_seed.defaultFlushMs = kFallbackIntervalMs;
        qCDebug(logPlayback) << "tracking uses fallback flush interval";
    }
    if (m_seed.scheduledFlushMs.isEmpty())
        qCDebug(logPlayback) << "tracking uses interval without early flush offsets";
    schedule();
}

void PlaybackTracker::flush(bool terminal)
{
    if (m_nonce.isEmpty() || m_seed.watchtimeUrl.isEmpty() || m_position <= m_anchor)
        return;
    QUrl url = requestUrl(m_seed.watchtimeUrl);
    QUrlQuery query(url);
    query.addQueryItem(QStringLiteral("st"), seconds(m_anchor));
    query.addQueryItem(QStringLiteral("et"), seconds(m_position));
    if (terminal)
        query.addQueryItem(QStringLiteral("final"), QStringLiteral("1"));
    url.setQuery(query);
    m_anchor = m_position;
    qCDebug(logPlayback) << (terminal ? "tracking final sent" : "tracking delta sent");
    Q_EMIT pingRequested(m_seed.clientKey, url);
}

void PlaybackTracker::schedule()
{
    if (m_seed.watchtimeUrl.isEmpty())
        return;
    const qint64 elapsed = m_wallClock.elapsed();
    for (const qint64 offset : m_seed.scheduledFlushMs) {
        if (offset > elapsed) {
            m_timer.start(int(offset - elapsed));
            return;
        }
    }
    const qint64 origin =
        m_seed.scheduledFlushMs.isEmpty() ? 0 : m_seed.scheduledFlushMs.constLast();
    const qint64 next =
        origin + ((elapsed - origin) / m_seed.defaultFlushMs + 1) * m_seed.defaultFlushMs;
    m_timer.start(int(next - elapsed));
}

}