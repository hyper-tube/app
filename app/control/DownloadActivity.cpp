#include "DownloadActivity.h"

#include "ControlCenter.h"
#include "library/Downloads.h"

#include <QDateTime>
#include <QStringList>

namespace {

constexpr int kSettleMs = 3000;

}

namespace control {

DownloadActivity::DownloadActivity(QObject *parent)
    : QObject(parent)
{
    m_settleTimer.setSingleShot(true);
    m_settleTimer.setInterval(kSettleMs);
    connect(&m_settleTimer, &QTimer::timeout, this, &DownloadActivity::settle);

    library::Downloads const &downloads = library::Downloads::instance();
    connect(&downloads, &library::Downloads::changed, this, &DownloadActivity::refresh);
    connect(&downloads, &library::Downloads::progressChanged, this, &DownloadActivity::refresh);
    connect(&downloads, &library::Downloads::catalogueChanged, this,
            &DownloadActivity::clearFailures);
    connect(&downloads, &library::Downloads::smartStored, this, &DownloadActivity::noteStored);
    connect(&downloads, &library::Downloads::smartFailed, this, &DownloadActivity::noteFailure);
    refresh();
}

void DownloadActivity::refresh()
{
    const library::Downloads &downloads = library::Downloads::instance();
    if (downloads.activeTitle().isEmpty() || downloads.activeForced()) {
        if (m_busy && !m_settleTimer.isActive())
            m_settleTimer.start();
        return;
    }
    m_settleTimer.stop();
    if (!m_busy)
        adoptCard();
    m_busy = true;
    publishProgress();
}

void DownloadActivity::settle()
{
    m_busy = false;
    if (m_saved == 0 && m_failures == 0)
        ControlCenter::instance().retract(m_cardId);
    else
        publishSummary();
}

void DownloadActivity::noteStored()
{
    adoptCard();
    ++m_saved;
    if (!m_busy)
        publishSummary();
}

void DownloadActivity::noteFailure(const QString &title)
{
    adoptCard();
    if (m_failures == 0)
        m_failureTitle = title.isEmpty() ? tr("A song") : title;
    ++m_failures;
    if (!m_busy)
        publishSummary();
}

void DownloadActivity::clearFailures()
{
    if (m_failures == 0)
        return;
    m_failures = 0;
    m_failureTitle.clear();
    if (m_busy || !ControlCenter::instance().contains(m_cardId))
        return;
    if (m_saved == 0)
        ControlCenter::instance().retract(m_cardId);
    else
        publishSummary();
}

void DownloadActivity::adoptCard()
{
    if (!m_cardId.isEmpty() && ControlCenter::instance().contains(m_cardId))
        return;
    m_cardId = QStringLiteral("downloads.smart.")
        + QString::number(QDateTime::currentMSecsSinceEpoch());
    m_saved = 0;
    m_failures = 0;
    m_failureTitle.clear();
}

void DownloadActivity::publishProgress()
{
    const library::Downloads &downloads = library::Downloads::instance();
    const int remaining = downloads.smartPending();
    Notification entry;
    entry.id = m_cardId;
    entry.activity = true;
    entry.title = tr("Smart downloads");
    entry.body = remaining > 1
        ? tr("%1 and %n more", nullptr, remaining - 1).arg(downloads.activeTitle())
        : downloads.activeTitle();
    entry.progress = downloads.fileProgress();
    ControlCenter::instance().publish(entry);
}

void DownloadActivity::publishSummary()
{
    Notification entry;
    entry.id = m_cardId;
    entry.icon = m_failures > 0 ? QStringLiteral("cloud_off") : QStringLiteral("download_done");
    entry.title = tr("Smart downloads");
    entry.body = summary();
    entry.quiet = true;
    ControlCenter::instance().publish(entry);
}

QString DownloadActivity::summary() const
{
    QStringList parts;
    if (m_saved > 0)
        parts.append(tr("%n songs saved for offline listening.", nullptr, m_saved));
    if (m_failures > 1)
        parts.append(tr("%n songs could not be downloaded. They are retried automatically.",
                        nullptr, m_failures));
    else if (m_failures == 1)
        parts.append(tr("**%1** could not be downloaded. It is retried automatically.")
                         .arg(m_failureTitle));
    return parts.join(QLatin1Char(' '));
}

}
