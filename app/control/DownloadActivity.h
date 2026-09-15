#pragma once

#include <QObject>
#include <QString>

namespace control {

class DownloadActivity : public QObject
{
    Q_OBJECT

public:
    explicit DownloadActivity(QObject *parent = nullptr);

private:
    void refresh();
    void noteFailure(const QString &title);
    void clearFailures();

    QString m_failureId;
    QString m_failureTitle;
    int m_failures = 0;
};

}
