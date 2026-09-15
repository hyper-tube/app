#include "Uploads.h"

#include "core/Logging.h"
#include "diagnostics/Diagnostics.h"
#include "innertube/Session.h"
#include "net/Connectivity.h"
#include "net/HttpClient.h"

#include <QCoreApplication>
#include <QFileInfo>

namespace {

constexpr qint64 kSizeLimit = 300LL * 1024 * 1024;

const QString kStartUrl =
    QStringLiteral("https://upload.youtube.com/upload/usermusic/http?authuser=0");
const QString kOrigin = QStringLiteral("https://music.youtube.com");
const QString kReferer = QStringLiteral("https://music.youtube.com/");
const QString kUserAgent =
    QStringLiteral("Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) "
                   "Chrome/152.0.0.0 Safari/537.36");

const QStringList kAcceptedExtensions {
    QStringLiteral("mp3"),  QStringLiteral("m4a"), QStringLiteral("aac"),
    QStringLiteral("flac"), QStringLiteral("ogg"), QStringLiteral("oga"),
    QStringLiteral("opus"), QStringLiteral("wav"), QStringLiteral("wma"),
};

net::Headers commonHeaders()
{
    return {
        {QByteArrayLiteral("origin"), kOrigin.toUtf8()},
        {QByteArrayLiteral("referer"), kReferer.toUtf8()},
        {QByteArrayLiteral("user-agent"), kUserAgent.toUtf8()},
    };
}

}

namespace library {

Uploads::Uploads(QObject *parent)
    : QObject(parent)
{
}

Uploads &Uploads::instance()
{
    static auto *uploads = new Uploads(QCoreApplication::instance());
    return *uploads;
}

Uploads *Uploads::create(QQmlEngine *, QJSEngine *)
{
    QQmlEngine::setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
    return &instance();
}

QStringList Uploads::extensions() const
{
    QStringList patterns;
    for (const QString &extension : kAcceptedExtensions)
        patterns.append(QStringLiteral("*.") + extension);
    return patterns;
}

double Uploads::progress() const
{
    if (m_batchTotal <= 1)
        return m_progress;
    return qBound(0.0, (m_batchDone + m_progress) / m_batchTotal, 1.0);
}

void Uploads::add(const QList<QUrl> &files)
{
    if (!innertube::Session::instance().authenticated()) {
        Q_EMIT feedback(tr("Sign in to upload your own music."), true);
        return;
    }
    if (!net::Connectivity::instance().online()) {
        Q_EMIT feedback(tr("Connect to the internet to upload music."), true);
        return;
    }

    int added = 0;
    for (const QUrl &file : files) {
        const QString path = file.isLocalFile() ? file.toLocalFile() : file.toString();
        const QFileInfo info(path);
        if (!info.isFile() || info.size() <= 0) {
            fail(info.fileName(), tr("it could not be read"));
            continue;
        }
        if (!kAcceptedExtensions.contains(info.suffix().toLower())) {
            fail(info.fileName(), tr("YouTube does not accept that format"));
            continue;
        }
        if (info.size() > kSizeLimit) {
            fail(info.fileName(), tr("it is larger than 300 MB"));
            continue;
        }
        m_queue.append(path);
        ++added;
    }

    if (added == 0)
        return;
    if (m_batchDone >= m_batchTotal) {
        m_batchTotal = 0;
        m_batchDone = 0;
        m_failures = 0;
    }
    m_batchTotal += added;
    Q_EMIT changed();
    pump();
}

void Uploads::pump()
{
    if (!m_activeName.isEmpty())
        return;
    if (m_queue.isEmpty()) {
        complete();
        return;
    }
    open(m_queue.takeFirst());
}

void Uploads::open(const QString &path)
{
    m_file.setFileName(path);
    const QString name = QFileInfo(path).fileName();
    if (!m_file.open(QIODevice::ReadOnly) || m_file.size() <= 0 || m_file.size() > kSizeLimit) {
        fail(name,
             m_file.size() > kSizeLimit ? tr("it is larger than 300 MB")
                                        : tr("it could not be read"));
        m_file.close();
        ++m_failures;
        ++m_batchDone;
        pump();
        return;
    }

    m_activeName = name;
    m_progress = 0;
    Q_EMIT changed();
    Q_EMIT progressChanged();

    net::Headers headers = commonHeaders();
    headers.append({QByteArrayLiteral("content-type"),
                    QByteArrayLiteral("application/x-www-form-urlencoded;charset=UTF-8")});
    headers.append({QByteArrayLiteral("x-goog-upload-command"), QByteArrayLiteral("start")});
    headers.append({QByteArrayLiteral("x-goog-upload-protocol"), QByteArrayLiteral("resumable")});
    headers.append({QByteArrayLiteral("x-goog-upload-header-content-length"),
                    QByteArray::number(m_file.size())});

    const QByteArray form = QByteArrayLiteral("filename=") + QUrl::toPercentEncoding(name);
    net::HttpClient::instance().post(QUrl(kStartUrl), headers, form, net::Credentialed::Yes,
                                     [this, path](const net::Response &response) {
        const QUrl target(
            QString::fromUtf8(response.header(QByteArrayLiteral("x-goog-upload-url"))));
        if (!response.ok() || target.isEmpty()) {
            m_file.close();
            diagnostics::breadcrumb("upload.failed",
                                    {{"stage", "session"}, {"status", response.status}},
                                    diagnostics::Level::Warning);
            fail(m_activeName, tr("YouTube refused the upload session"));
            m_activeName.clear();
            ++m_failures;
            ++m_batchDone;
            Q_EMIT changed();
            pump();
            return;
        }
        send(path, target);
    });
}

void Uploads::send(const QString &path, const QUrl &target)
{
    net::Headers headers = commonHeaders();
    headers.append({QByteArrayLiteral("content-type"),
                    QByteArrayLiteral("application/x-www-form-urlencoded;charset=utf-8")});
    headers.append(
        {QByteArrayLiteral("x-goog-upload-command"), QByteArrayLiteral("upload, finalize")});
    headers.append({QByteArrayLiteral("x-goog-upload-offset"), QByteArrayLiteral("0")});

    net::HttpClient::instance().upload(target, headers, &m_file, [this](qint64 sent, qint64 total) {
        m_progress = total > 0 ? qBound(0.0, double(sent) / total, 1.0) : 0;
        Q_EMIT progressChanged();
    }, [this, path](const net::Response &response) {
        m_file.close();
        const QByteArray status = response.header(QByteArrayLiteral("x-goog-upload-status"));
        if (!response.ok() || status != QByteArrayLiteral("final")) {
            diagnostics::breadcrumb("upload.failed",
                                    {{"stage", "transfer"}, {"status", response.status}},
                                    diagnostics::Level::Warning);
            fail(m_activeName, tr("YouTube did not accept the file"));
            ++m_failures;
        } else {
            qCInfo(logNet) << "uploaded" << QFileInfo(path).fileName();
        }
        m_activeName.clear();
        m_progress = 0;
        ++m_batchDone;
        Q_EMIT changed();
        Q_EMIT progressChanged();
        pump();
    });
}

void Uploads::fail(const QString &name, const QString &reason)
{
    Q_EMIT feedback(tr("%1 was not uploaded because %2.").arg(name, reason), true);
}

void Uploads::complete()
{
    const int uploaded = m_batchDone - m_failures;
    m_batchTotal = 0;
    m_batchDone = 0;
    m_failures = 0;
    Q_EMIT changed();
    Q_EMIT progressChanged();
    if (uploaded <= 0)
        return;
    Q_EMIT feedback(
        tr("Uploaded %n tracks. YouTube takes a few minutes to list them.", nullptr, uploaded),
        false);
}

}
