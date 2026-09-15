#include "Updater.h"

#include "ReleaseNotes.h"
#include "control/ControlCenter.h"
#include "core/AppInfo.h"
#include "core/Localization.h"
#include "core/Logging.h"
#include "core/Paths.h"
#include "diagnostics/Diagnostics.h"
#include "net/Connectivity.h"
#include "net/HttpClient.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QLocale>
#include <QRegularExpression>
#include <QSettings>
#include <QUrlQuery>

#ifdef Q_OS_WIN
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#endif

#include <algorithm>
#include <utility>

namespace {

constexpr auto kAutomaticKey = "updates/automatic";
constexpr auto kSeenKey = "updates/seen";
constexpr auto kPendingKey = "updates/pending";

constexpr int kFirstCheckDelayMs = 8000;
constexpr int kCheckIntervalMs = 6 * 60 * 60 * 1000;
constexpr int kHandOffDelayMs = 1200;

const QString kNoticePrefix = QStringLiteral("updates.release.");
const QString kUnfinishedPrefix = QStringLiteral("updates.unfinished.");
const QString kDownloadActivityId = QStringLiteral("updates.download");

QString packageFolder()
{
    return QDir(core::paths::cacheDir()).filePath(QStringLiteral("updates"));
}

QString setupLog()
{
    return QDir(packageFolder()).filePath(QStringLiteral("setup.log"));
}

void discardPackages()
{
    QDir folder(packageFolder());
    const QStringList packages = folder.entryList({QStringLiteral("*.exe")}, QDir::Files);
    for (const QString &name : packages)
        folder.remove(name);
}

QByteArray userAgent()
{
    return (core::AppInfo::identifier() + QLatin1Char('/') + core::AppInfo::version()).toLatin1();
}

QString readableSize(qint64 bytes)
{
    return QLocale().formattedDataSize(bytes, 1, QLocale::DataSizeTraditionalFormat);
}

const QRegularExpression &weekdayPattern()
{
    static const QRegularExpression pattern(QStringLiteral("\\s*,?\\s*dddd\\s*,?\\s*"));
    return pattern;
}

#ifdef Q_OS_WIN
bool managedInstallation()
{
    return QFile::exists(
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("install.json")));
}
#endif

}

namespace update {

Updater::Updater(QObject *parent)
    : QObject(parent)
    , m_running(SemanticVersion::parse(core::AppInfo::version()))
    , m_automatic(QSettings().value(kAutomaticKey, true).toBool())
{
    m_schedule.setInterval(kCheckIntervalMs);
    connect(&m_schedule, &QTimer::timeout, this, &Updater::check);

    const control::ControlCenter &center = control::ControlCenter::instance();
    connect(&center, &control::ControlCenter::actionInvoked, this, [this](const QString &id) {
        if (m_release && id == noticeId())
            showDetails();
    });
    connect(&center, &control::ControlCenter::changed, this, [this] {
        if (m_release && !control::ControlCenter::instance().unread(noticeId()))
            rememberSeen();
    });
    connect(&net::Connectivity::instance(), &net::Connectivity::onlineChanged, this,
            &Updater::checkIfDue);
    connect(&core::Localization::instance(), &core::Localization::resolvedChanged, this, [this] {
        if (!m_release)
            return;
        m_sections = releaseNotes::parse(m_release->notes);
        Q_EMIT releaseChanged();
        announce();
    });
}

Updater &Updater::instance()
{
    static auto *updater = new Updater(QCoreApplication::instance());
    return *updater;
}

Updater *Updater::create(QQmlEngine *, QJSEngine *)
{
    QQmlEngine::setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
    return &instance();
}

void Updater::start()
{
    QSettings settings;
    const QString pending = settings.value(kPendingKey).toString();
    settings.remove(kPendingKey);
    if (SemanticVersion::parse(pending) > m_running)
        reportUnfinished(pending);
    discardPackages();
    QTimer::singleShot(kFirstCheckDelayMs, this, &Updater::schedule);
}

QString Updater::version() const
{
    return m_release ? m_release->version.toString() : QString();
}

QString Updater::published() const
{
    if (!m_release || !m_release->publishedAt.isValid())
        return {};
    const QLocale locale;
    QString format = locale.dateFormat(QLocale::LongFormat);
    format.replace(weekdayPattern(), QStringLiteral(" "));
    return locale.toString(m_release->publishedAt.toLocalTime().date(), format.trimmed());
}

bool Updater::prerelease() const
{
    return m_release && m_release->prerelease;
}

QString Updater::notes() const
{
    return m_release ? m_release->notes : QString();
}

QUrl Updater::changelogUrl() const
{
    return m_release ? m_release->changelogUrl : QUrl();
}

QUrl Updater::downloadPage() const
{
    if (!m_release)
        return {};
    return QUrl(core::AppInfo::website() + QStringLiteral("/download/")
                + m_release->version.toString());
}

bool Updater::installable() const
{
#ifdef Q_OS_WIN
    return m_release && m_release->windowsInstaller.verifiable() && managedInstallation();
#else
    return false;
#endif
}

double Updater::progress() const
{
    if (!m_release || m_release->windowsInstaller.size <= 0)
        return 0;
    return std::clamp(double(m_received) / double(m_release->windowsInstaller.size), 0.0, 1.0);
}

QString Updater::transferred() const
{
    if (!m_release)
        return {};
    return tr("%1 of %2")
        .arg(readableSize(m_received), readableSize(m_release->windowsInstaller.size));
}

void Updater::setAutomatic(bool automatic)
{
    if (m_automatic == automatic)
        return;
    m_automatic = automatic;
    QSettings().setValue(kAutomaticKey, automatic);
    Q_EMIT automaticChanged();
    schedule();
}

void Updater::check()
{
    if (m_checkState == Checking)
        return;
    setCheckState(Checking);

    QUrl url(core::AppInfo::website() + QStringLiteral("/api/releases/latest"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("channel"), channel());
    url.setQuery(query);

    net::Headers headers {{QByteArrayLiteral("Accept"), QByteArrayLiteral("application/json")},
                          {QByteArrayLiteral("User-Agent"), userAgent()}};
    if (!m_entityTag.isEmpty())
        headers.append(net::Header {QByteArrayLiteral("If-None-Match"), m_entityTag});

    const QPointer<Updater> guard(this);
    net::HttpClient::instance().get(url, headers, net::Credentialed::No,
                                    [this, guard](const net::Response &response) {
        if (guard)
            receive(response);
    }, true);
}

void Updater::install()
{
    if (!m_release || busy())
        return;
    if (!installable()) {
        QDesktopServices::openUrl(downloadPage());
        return;
    }

    const QDir folder(packageFolder());
    if (!folder.mkpath(QStringLiteral("."))) {
        setStage(Failed,
                 tr("The update could not be saved, because the cache folder is not "
                    "writable."));
        return;
    }
    discardPackages();
    rememberSeen();

    const Release::Package &package = m_release->windowsInstaller;
    m_received = 0;
    m_download = new PackageDownload(package, folder.filePath(package.fileName), this);
    connect(m_download, &PackageDownload::progress, this, [this](qint64 received, qint64) {
        m_received = received;
        Q_EMIT progressChanged();
        publishProgress();
    });
    connect(m_download, &PackageDownload::finished, this, &Updater::conclude);
    setStage(Downloading);
    Q_EMIT progressChanged();
    publishProgress();
    m_download->start();
}

void Updater::cancel()
{
    if (m_stage == Downloading && m_download)
        m_download->cancel();
    else if (m_stage == Failed)
        setStage(Idle);
}

void Updater::showDetails()
{
    if (!m_release)
        return;
    rememberSeen();
    Q_EMIT detailsRequested();
}

bool Updater::busy() const
{
    return m_stage == Downloading || m_stage == Installing;
}

QString Updater::channel() const
{
    return m_running.prerelease() ? QStringLiteral("prerelease") : QStringLiteral("stable");
}

QString Updater::noticeId() const
{
    return m_release ? kNoticePrefix + m_release->version.toString() : QString();
}

void Updater::schedule()
{
    if (!m_automatic) {
        m_schedule.stop();
        return;
    }
    if (!m_schedule.isActive())
        m_schedule.start();
    checkIfDue();
}

void Updater::checkIfDue()
{
    if (!m_automatic || !m_schedule.isActive() || m_checkState == Checking
        || !net::Connectivity::instance().online()) {
        return;
    }
    if (m_checkState == Checked && m_checkedAt.isValid()
        && m_checkedAt.msecsTo(QDateTime::currentDateTimeUtc()) < kCheckIntervalMs) {
        return;
    }
    check();
}

void Updater::receive(const net::Response &response)
{
    if (response.status == 304) {
        m_checkedAt = QDateTime::currentDateTimeUtc();
        setCheckState(Checked);
        return;
    }
    if (response.status == 404) {
        m_entityTag.clear();
        m_checkedAt = QDateTime::currentDateTimeUtc();
        if (!busy())
            adopt(std::nullopt);
        setCheckState(Checked);
        return;
    }

    std::optional<Release> release;
    if (response.ok())
        release = Release::fromJson(response.body);
    if (!release) {
        setCheckState(Unreachable);
        return;
    }

    m_checkedAt = QDateTime::currentDateTimeUtc();
    if (busy()) {
        m_entityTag.clear();
    } else {
        m_entityTag = response.header("etag");
        adopt(std::move(release));
    }
    setCheckState(Checked);
}

void Updater::adopt(std::optional<Release> release)
{
    if (release && release->version <= m_running)
        release.reset();

    const bool sameVersion = release && m_release && m_release->version == release->version;
    if (!sameVersion)
        retractNotice();
    if (!release) {
        if (m_release) {
            m_release.reset();
            m_sections.clear();
            Q_EMIT releaseChanged();
        }
        return;
    }

    if (!sameVersion) {
        qCInfo(logNet) << "version" << release->version.toString() << "is available";
        diagnostics::breadcrumb("update.available", {{"prerelease", release->prerelease}});
    }
    m_sections = releaseNotes::parse(release->notes);
    m_release = std::move(release);
    Q_EMIT releaseChanged();
    announce();
}

void Updater::retractNotice()
{
    if (m_release)
        control::ControlCenter::instance().retract(noticeId());
}

void Updater::announce()
{
    if (!m_release)
        return;
    const QString version = m_release->version.toString();
    control::Notification entry;
    entry.id = noticeId();
    entry.icon = QStringLiteral("deployed_code_update");
    entry.title = tr("Update available");
    entry.body = installable()
        ? tr("**%1 %2** is ready to install.").arg(core::AppInfo::name(), version)
        : tr("**%1 %2** is available to download.").arg(core::AppInfo::name(), version);
    entry.action = tr("See what's new");
    entry.actionIcon = QStringLiteral("arrow_forward");
    entry.quiet = QSettings().value(kSeenKey).toString() == version;
    control::ControlCenter::instance().publish(entry);
}

void Updater::rememberSeen() const
{
    if (!m_release)
        return;
    QSettings settings;
    const QString version = m_release->version.toString();
    if (settings.value(kSeenKey).toString() != version)
        settings.setValue(kSeenKey, version);
}

void Updater::publishProgress()
{
    if (m_stage != Downloading || !m_release)
        return;
    control::Notification entry;
    entry.id = kDownloadActivityId;
    entry.activity = true;
    entry.title = tr("Downloading %1 %2").arg(core::AppInfo::name(), version());
    entry.body = transferred();
    entry.progress = progress();
    control::ControlCenter::instance().publish(entry);
}

void Updater::conclude(update::PackageDownload::Failure failure)
{
    control::ControlCenter::instance().retract(kDownloadActivityId);
    const QString package = m_download ? m_download->path() : QString();
    if (m_download)
        m_download->deleteLater();

    switch (failure) {
    case PackageDownload::Failure::None:
        setStage(Installing);
        QTimer::singleShot(kHandOffDelayMs, this, [this, package] { handOff(package); });
        return;
    case PackageDownload::Failure::Cancelled: setStage(Idle); return;
    case PackageDownload::Failure::Unreachable:
        setStage(Failed,
                 tr("The download stopped because the network went away. Try again once "
                    "you are back online."));
        return;
    case PackageDownload::Failure::Refused:
        setStage(Failed, tr("The download server refused the update. Try again later."));
        return;
    case PackageDownload::Failure::Oversized:
    case PackageDownload::Failure::Incomplete:
        setStage(Failed, tr("The update did not arrive complete, so it was discarded."));
        return;
    case PackageDownload::Failure::Mismatch:
        setStage(Failed,
                 tr("The downloaded installer did not match its published checksum, so "
                    "it was deleted without running."));
        return;
    case PackageDownload::Failure::Storage:
        setStage(Failed, tr("The update could not be saved to disk."));
        return;
    }
}

void Updater::handOff(const QString &package)
{
#ifdef Q_OS_WIN
    const QStringList arguments {
        QStringLiteral("/UPDATE"),
        QStringLiteral("/VERYSILENT"),
        QStringLiteral("/CLOSEAPPLICATIONS"),
        QStringLiteral("/LAUNCH"),
        QStringLiteral("/DIR=") + QDir::toNativeSeparators(QCoreApplication::applicationDirPath()),
        QStringLiteral("/LOG=") + QDir::toNativeSeparators(setupLog()),
    };
    if (!QProcess::startDetached(package, arguments, QFileInfo(package).absolutePath())) {
        QFile::remove(package);
        setStage(Failed, tr("The installer could not be started."));
        return;
    }
    QSettings().setValue(kPendingKey, m_release->version.toString());
    qCInfo(logPlatform) << "installer started for" << m_release->version.toString() << "quitting";
    diagnostics::breadcrumb("update.installer_started");
    QCoreApplication::quit();
#else
    Q_UNUSED(package)
    setStage(Idle);
#endif
}

void Updater::reportUnfinished(const QString &version)
{
    qCWarning(logPlatform) << "the update to" << version << "did not finish";
    diagnostics::breadcrumb("update.unfinished", {}, diagnostics::Level::Warning);

    control::Notification entry;
    entry.id = kUnfinishedPrefix + version;
    entry.icon = QStringLiteral("sync_problem");
    entry.title = tr("The update did not finish");
    entry.body = tr("%1 is still on version %2. Setup kept a log at `%3`.")
                     .arg(core::AppInfo::name(), core::AppInfo::version(),
                          QDir::toNativeSeparators(setupLog()));
    control::ControlCenter::instance().publish(entry);
}

void Updater::setCheckState(CheckState state)
{
    if (m_checkState == state)
        return;
    m_checkState = state;
    Q_EMIT checkStateChanged();
}

void Updater::setStage(Stage stage, const QString &error)
{
    if (m_stage == stage && m_error == error)
        return;
    m_stage = stage;
    m_error = error;
    Q_EMIT stageChanged();
}

}
