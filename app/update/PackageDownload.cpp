#include "PackageDownload.h"

#include "core/AppInfo.h"
#include "core/Logging.h"
#include "diagnostics/Diagnostics.h"
#include "net/Connectivity.h"

#include <QNetworkReply>
#include <QNetworkRequest>

#include <utility>

namespace {

constexpr int kStallTimeoutMs = 30000;
constexpr qint64 kChunkBytes = 512 * 1024;

const char *reasonOf(update::PackageDownload::Failure failure)
{
    switch (failure) {
    case update::PackageDownload::Failure::None: return "none";
    case update::PackageDownload::Failure::Cancelled: return "cancelled";
    case update::PackageDownload::Failure::Unreachable: return "unreachable";
    case update::PackageDownload::Failure::Refused: return "refused";
    case update::PackageDownload::Failure::Oversized: return "oversized";
    case update::PackageDownload::Failure::Incomplete: return "incomplete";
    case update::PackageDownload::Failure::Mismatch: return "checksum_mismatch";
    case update::PackageDownload::Failure::Storage: return "storage";
    }
    return "unknown";
}

}

namespace update {

PackageDownload::PackageDownload(Release::Package package, const QString &path, QObject *parent)
    : QObject(parent)
    , m_file(path)
    , m_digest(QCryptographicHash::Sha256)
    , m_package(std::move(package))
    , m_path(path)
{
    m_file.setDirectWriteFallback(false);
}

void PackageDownload::start()
{
    if (m_reply)
        return;
    if (!m_file.open(QIODevice::WriteOnly)) {
        fail(Failure::Storage, m_file.errorString());
        Q_EMIT finished(m_failure);
        return;
    }

    QNetworkRequest request(m_package.url);
    request.setTransferTimeout(kStallTimeoutMs);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setRawHeader(
        QByteArrayLiteral("User-Agent"),
        (core::AppInfo::identifier() + QLatin1Char('/') + core::AppInfo::version()).toLatin1());

    qCInfo(logNet) << "downloading update package" << m_package.fileName << m_package.size
                   << "bytes";
    m_reply = m_manager.get(request);
    connect(m_reply, &QIODevice::readyRead, this, &PackageDownload::read);
    connect(m_reply, &QNetworkReply::downloadProgress, this,
            [this](qint64 received, qint64) { Q_EMIT progress(received, m_package.size); });
    connect(m_reply, &QNetworkReply::finished, this, &PackageDownload::conclude);
}

void PackageDownload::cancel()
{
    if (!m_reply || m_reply->isFinished())
        return;
    fail(Failure::Cancelled);
}

void PackageDownload::fail(Failure failure, const QString &detail)
{
    if (m_failure != Failure::None)
        return;
    m_failure = failure;
    m_detail = detail;
    if (m_reply && !m_reply->isFinished())
        m_reply->abort();
}

void PackageDownload::read()
{
    if (m_failure != Failure::None || !m_reply)
        return;
    const int status = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status != 200) {
        fail(Failure::Refused, QString::number(status));
        return;
    }
    while (m_reply->bytesAvailable() > 0) {
        const QByteArray bytes = m_reply->read(kChunkBytes);
        m_received += bytes.size();
        if (m_received > m_package.size) {
            fail(Failure::Oversized);
            return;
        }
        m_digest.addData(bytes);
        if (m_file.write(bytes) != bytes.size()) {
            fail(Failure::Storage, m_file.errorString());
            return;
        }
    }
}

void PackageDownload::conclude()
{
    const QNetworkReply::NetworkError error = m_reply->error();
    const int status = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QString complaint = m_reply->errorString();
    if (error == QNetworkReply::NoError)
        read();
    m_reply->deleteLater();

    if (m_failure != Failure::Cancelled)
        net::Connectivity::instance().observe(error, status);
    if (m_failure == Failure::None && error != QNetworkReply::NoError) {
        fail(status == 0 && net::Connectivity::unreachable(error) ? Failure::Unreachable
                                                                  : Failure::Refused,
             status > 0 ? QString::number(status) : complaint);
    }
    if (m_failure == Failure::None && m_received != m_package.size)
        fail(Failure::Incomplete);
    if (m_failure == Failure::None && m_digest.result().toHex() != m_package.sha256)
        fail(Failure::Mismatch);
    if (m_failure == Failure::None && !m_file.commit())
        fail(Failure::Storage, m_file.errorString());

    if (m_failure == Failure::None) {
        qCInfo(logNet) << "update package verified" << m_package.fileName;
    } else {
        m_file.cancelWriting();
        qCWarning(logNet) << "update package discarded" << reasonOf(m_failure) << m_detail;
        if (m_failure != Failure::Cancelled)
            diagnostics::breadcrumb("update.download_failed",
                                    {{"reason", reasonOf(m_failure)}, {"status", status}},
                                    diagnostics::Level::Warning);
    }
    Q_EMIT finished(m_failure);
}

}
