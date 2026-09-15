#pragma once

#include "Release.h"

#include <QCryptographicHash>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QSaveFile>

class QNetworkReply;

namespace update {

class PackageDownload : public QObject
{
    Q_OBJECT

public:
    enum class Failure {
        None,
        Cancelled,
        Unreachable,
        Refused,
        Oversized,
        Incomplete,
        Mismatch,
        Storage,
    };
    Q_ENUM(Failure)

    PackageDownload(Release::Package package, const QString &path, QObject *parent);

    void start();
    void cancel();

    const QString &path() const { return m_path; }
    const QString &detail() const { return m_detail; }

Q_SIGNALS:
    void progress(qint64 received, qint64 total);
    void finished(update::PackageDownload::Failure failure);

private:
    void read();
    void conclude();
    void fail(Failure failure, const QString &detail = {});

    QNetworkAccessManager m_manager;
    QSaveFile m_file;
    QCryptographicHash m_digest;
    QPointer<QNetworkReply> m_reply;
    Release::Package m_package;
    QString m_path;
    QString m_detail;
    qint64 m_received = 0;
    Failure m_failure = Failure::None;
};

}
