#pragma once

#include <QFile>
#include <QObject>
#include <QPointer>

class QNetworkReply;

namespace net {

class DownloadTransfer : public QObject
{
    Q_OBJECT

public:
    DownloadTransfer(const QString &path, qint64 offset, qint64 maximumBytes, QObject *parent);

    void start(QNetworkReply *reply);
    void cancel();
    bool restartRequired() const { return m_restartRequired; }
    bool cancelled() const { return m_cancelled; }
    bool unreachable() const { return m_unreachable; }

Q_SIGNALS:
    void progress(qint64 received, qint64 total);
    void finished(const QString &error);

private:
    void read();
    bool open();
    void fail(const QString &error);

    QFile m_file;
    QPointer<QNetworkReply> m_reply;
    QString m_error;
    qint64 m_offset;
    qint64 m_maximumBytes;
    qint64 m_expectedSize = 0;
    bool m_restartRequired = false;
    bool m_cancelled = false;
    bool m_unreachable = false;
};

}
