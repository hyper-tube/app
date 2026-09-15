#include "DownloadTransfer.h"

#include "Connectivity.h"
#include "diagnostics/Diagnostics.h"

#include <QMetaEnum>
#include <QNetworkReply>
#include <QRegularExpression>
#include <cstddef>

namespace net {

DownloadTransfer::DownloadTransfer(const QString &path, qint64 offset, qint64 maximumBytes,
                                   QObject *parent)
    : QObject(parent)
    , m_file(path)
    , m_offset(offset)
    , m_maximumBytes(maximumBytes)
{
}

void DownloadTransfer::start(QNetworkReply *reply)
{
    m_reply = reply;
    connect(reply, &QIODevice::readyRead, this, &DownloadTransfer::read);
    connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        Q_EMIT progress(m_offset + received, total > 0 ? m_offset + total : 0);
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (!m_cancelled && m_offset > 0 && status == 416) {
            m_file.close();
            m_file.remove();
            m_restartRequired = true;
        }
        if (!m_cancelled) {
            Connectivity::instance().observe(reply->error(), status);
            m_unreachable =
                m_error.isEmpty() && status == 0 && Connectivity::unreachable(reply->error());
        }
        if (reply->error() == QNetworkReply::NoError)
            read();
        if (m_error.isEmpty() && reply->error() != QNetworkReply::NoError) {
            m_error = status >= 400
                ? tr("The audio server refused the download (HTTP %1).").arg(status)
                : tr("The transfer was interrupted. Retry to resume it.");
        }
        if (m_error.isEmpty() && (!m_file.isOpen() || m_file.size() == 0))
            m_error = tr("The server returned no audio.");
        if (m_error.isEmpty() && m_expectedSize > 0 && m_file.size() != m_expectedSize)
            m_error = tr("The audio transfer was incomplete. Retry to resume it.");
        if (m_file.isOpen() && !m_file.flush() && m_error.isEmpty())
            m_error = m_file.errorString();
        m_file.close();
        reply->deleteLater();
        if (!m_cancelled && !m_error.isEmpty())
            diagnostics::breadcrumb(
                "download.transfer_failed",
                {{"status", status},
                 {"reason",
                  QMetaEnum::fromType<QNetworkReply::NetworkError>().valueToKey(reply->error())},
                 {"resumed", m_offset > 0},
                 {"unreachable", m_unreachable}},
                diagnostics::Level::Warning);
        Q_EMIT finished(m_error);
    });
}

void DownloadTransfer::cancel()
{
    if (m_cancelled)
        return;
    m_cancelled = true;
    m_error = tr("Download cancelled.");
    if (m_reply)
        m_reply->abort();
    else
        Q_EMIT finished(m_error);
}

void DownloadTransfer::fail(const QString &error)
{
    m_error = error;
    if (m_reply && !m_reply->isFinished())
        m_reply->abort();
}

bool DownloadTransfer::open()
{
    const int status = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status != 200 && status != 206) {
        fail(tr("The audio server refused the download."));
        return false;
    }
    if (status == 200) {
        m_offset = 0;
        m_expectedSize = m_reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
    }
    if (status == 206) {
        static const QRegularExpression range(QStringLiteral("^bytes (\\d+)-(\\d+)/(\\d+)$"));
        const auto match = range.match(QString::fromLatin1(m_reply->rawHeader("content-range")));
        if (!match.hasMatch() || match.captured(1).toLongLong() != m_offset
            || match.captured(2).toLongLong() + 1 != match.captured(3).toLongLong()) {
            fail(tr("The server returned an unexpected audio range."));
            return false;
        }
        m_expectedSize = match.captured(3).toLongLong();
    }
    const qint64 length = m_reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
    if (length > m_maximumBytes - m_offset) {
        fail(tr("The download exceeds the storage ceiling."));
        return false;
    }
    if (!m_file.open(QIODevice::WriteOnly
                     | (m_offset > 0 ? QIODevice::Append : QIODevice::Truncate))) {
        fail(m_file.errorString());
        return false;
    }
    return true;
}

void DownloadTransfer::read()
{
    if (!m_error.isEmpty() || !m_reply || m_reply->bytesAvailable() == 0)
        return;
    if (!m_file.isOpen() && !open())
        return;
    while (m_reply->bytesAvailable() > 0) {
        const QByteArray bytes = m_reply->read(256 * 1024);
        if (bytes.size() > m_maximumBytes - m_file.size()) {
            fail(tr("The download exceeds the storage ceiling."));
            return;
        }
        if (m_file.write(bytes) != bytes.size()) {
            fail(m_file.errorString());
            return;
        }
    }
}

}
