#include "DownloadActivity.h"

#include "ControlCenter.h"
#include "library/Downloads.h"

#include <QDateTime>

namespace {

const QString kActivityId = QStringLiteral("downloads.smart");

}

namespace control {

DownloadActivity::DownloadActivity(QObject *parent)
    : QObject(parent)
{
    library::Downloads const &downloads = library::Downloads::instance();
    connect(&downloads, &library::Downloads::changed, this, &DownloadActivity::refresh);
    connect(&downloads, &library::Downloads::progressChanged, this, &DownloadActivity::refresh);
    connect(&downloads, &library::Downloads::catalogueChanged, this,
            &DownloadActivity::clearFailures);
    connect(&downloads, &library::Downloads::smartFailed, this, &DownloadActivity::noteFailure);
    refresh();
}

void DownloadActivity::refresh()
{
    const library::Downloads &downloads = library::Downloads::instance();
    if (downloads.activeTitle().isEmpty() || downloads.activeForced()) {
        ControlCenter::instance().retract(kActivityId);
        return;
    }
    const int remaining = downloads.smartPending();
    Notification entry;
    entry.id = kActivityId;
    entry.activity = true;
    entry.title = tr("Smart downloads");
    entry.body = remaining > 1
        ? tr("%1 and %n more", nullptr, remaining - 1).arg(downloads.activeTitle())
        : downloads.activeTitle();
    entry.progress = downloads.fileProgress();
    ControlCenter::instance().publish(entry);
}

void DownloadActivity::noteFailure(const QString &title)
{
    if (m_failures == 0) {
        m_failureId = QStringLiteral("downloads.failed.")
            + QString::number(QDateTime::currentSecsSinceEpoch());
        m_failureTitle = title.isEmpty() ? tr("A song") : title;
    }
    ++m_failures;
    Notification entry;
    entry.id = m_failureId;
    entry.icon = QStringLiteral("cloud_off");
    entry.title = tr("Downloads did not finish");
    entry.body = m_failures > 1
        ? tr("%n songs could not be downloaded. They are retried automatically.", nullptr,
             m_failures)
        : tr("**%1** could not be downloaded. It is retried automatically.").arg(m_failureTitle);
    ControlCenter::instance().publish(entry);
}

void DownloadActivity::clearFailures()
{
    if (m_failures == 0)
        return;
    m_failures = 0;
    ControlCenter::instance().retract(m_failureId);
    m_failureId.clear();
    m_failureTitle.clear();
}

}
