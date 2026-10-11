#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

namespace control {

class DownloadActivity : public QObject
{
    Q_OBJECT

public:
    explicit DownloadActivity(QObject *parent = nullptr);

private:
    void refresh();
    void settle();
    void noteStored();
    void noteFailure(const QString &title);
    void clearFailures();
    void adoptCard();
    void publishProgress();
    void publishSummary();
    QString summary() const;

    QTimer m_settleTimer;
    QString m_cardId;
    QString m_failureTitle;
    int m_saved = 0;
    int m_failures = 0;
    bool m_busy = false;
};

}
