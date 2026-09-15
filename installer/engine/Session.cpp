#include "Session.h"

#include "Archive.h"
#include "CommandLine.h"
#include "Product.h"
#include "Running.h"
#include "Manifest.h"
#include "SafePath.h"

#include "core/Localization.h"

#include "core/Logging.h"

#include <QCoreApplication>
#include <QClipboard>
#include <QGuiApplication>
#include <QOperatingSystemVersion>
#include <QTemporaryFile>
#include <QVersionNumber>
#include <QEventLoop>
#include <QTimer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QProcess>
#include <QStorageInfo>

#include <windows.h>

#include <shellapi.h>

namespace {

const QString kLicenseResource = QStringLiteral(":/setup/LICENSE");

bool insideSystemFolder(const QString &directory)
{
    wchar_t windows[MAX_PATH] {};
    GetWindowsDirectoryW(windows, MAX_PATH);
    const QStringList forbidden {QDir::fromNativeSeparators(QString::fromWCharArray(windows))};
    const QString clean = QDir::cleanPath(directory);
    for (const QString &path : forbidden) {
        if (clean.compare(path, Qt::CaseInsensitive) == 0
            || clean.startsWith(path + QLatin1Char('/'), Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

}

namespace setup {

Session::Session(QObject *parent)
    : QObject(parent)
{
}

Session &Session::instance()
{
    static auto *session = new Session(QCoreApplication::instance());
    return *session;
}

Session *Session::create(QQmlEngine *, QJSEngine *)
{
    QQmlEngine::setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
    return &instance();
}

void Session::adopt(const Options &options)
{
    m_options = options;
    HANDLE token = nullptr;
    TOKEN_ELEVATION elevation {};
    DWORD bytes = sizeof(elevation);
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        m_options.elevated =
            GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &bytes)
            && elevation.TokenIsElevated;
        CloseHandle(token);
    }
    m_options.directory = QDir::fromNativeSeparators(m_options.directory);
    ArchiveFooter originFooter;
    const bool validOrigin = ArchiveFooter::readFrom(
        std::filesystem::path(options.origin.toStdU16String()), originFooter);
    if (validOrigin && originFooter.payloadSize == 0) {
        m_options.mode = Mode::Uninstall;
        m_options.directory = QFileInfo(options.origin).absolutePath();
    }
    m_installed = options.scopeExplicit ? registration::find(options.scope) : registration::find();
    if (m_installed.present && !m_options.directory.isEmpty()
        && QDir::cleanPath(m_installed.directory)
                .compare(QDir::cleanPath(m_options.directory), Qt::CaseInsensitive)
            != 0) {
        m_installed = {};
    }
    const Manifest local = Manifest::read(m_options.directory);
    if (!m_installed.present && local.valid) {
        m_installed = {
            true,          local.scope, local.version, m_options.directory, local.components,
            local.language};
    }

    if (m_installed.present) {
        if (m_options.directory.isEmpty())
            m_options.directory = m_installed.directory;
        if (m_options.mode == Mode::Install)
            m_options.mode =
                m_installed.version == product::version() ? Mode::Repair : Mode::Update;
        m_options.scope = m_installed.scope;
        if (!options.componentsExplicit)
            m_options.components = m_installed.components;
    }

    if (m_options.directory.isEmpty())
        m_options.directory = product::defaultDirectory(m_options.scope);

    m_required = m_options.mode == Mode::Uninstall ? 0 : Job::requiredBytes(m_options);
    qCDebug(logPlatform) << "setup origin" << m_options.origin << "needs" << m_required
                         << "installed" << m_installed.present << m_installed.version;
    refreshBlockers();
    Q_EMIT changed();
    Q_EMIT directoryChanged();
}

bool Session::desktopShortcut() const
{
    return m_options.components.testFlag(Component::DesktopShortcut);
}

bool Session::startMenuShortcut() const
{
    return m_options.components.testFlag(Component::StartMenuShortcut);
}

bool Session::launchAtSignIn() const
{
    return m_options.components.testFlag(Component::LaunchAtSignIn);
}

bool Session::urlScheme() const
{
    return m_options.components.testFlag(Component::UrlScheme);
}

QString Session::productName() const
{
    return product::displayName();
}

QString Session::version() const
{
    return product::version();
}

QString Session::publisher() const
{
    return product::publisher();
}

QString Session::website() const
{
    return product::websiteUrl();
}

QString Session::licenseText() const
{
    QFile file(kLicenseResource);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QString::fromUtf8(file.readAll());
}

QString Session::urlSchemeName() const
{
    return product::urlScheme();
}

bool Session::elevationNeeded() const
{
    return m_options.scope == Scope::AllUsers && !m_options.elevated;
}

qint64 Session::availableBytes() const
{
    QString probe = m_options.directory;
    while (!probe.isEmpty() && !QFileInfo::exists(probe)) {
        const QString parent = QFileInfo(probe).absolutePath();
        if (parent == probe)
            break;
        probe = parent;
    }
    const QStorageInfo storage(probe);
    return storage.isValid() ? storage.bytesAvailable() : -1;
}

QString Session::directoryComplaint() const
{
    const QString clean = QDir::cleanPath(m_options.directory);
    if (clean.isEmpty())
        return tr("Choose a folder to install into.");
    if (QDir::isRelativePath(clean))
        return tr("The folder must be a full path, such as C:\\Programs\\%1.")
            .arg(product::folderName());
    if (insideSystemFolder(clean))
        return tr("Windows system folders cannot hold applications.");
    if (QDir(clean).isRoot() || !safeDestination(std::filesystem::path(clean.toStdU16String())))
        return tr("Choose a normal application folder, not a drive root or redirected folder.");
    if (QFileInfo::exists(clean) && !QFileInfo(clean).isDir())
        return tr("A file of that name is already there.");

    const QDir destination(clean);
    if (m_options.mode != Mode::Uninstall && destination.exists() && !Manifest::read(clean).valid
        && !destination
                .entryList(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot)
                .isEmpty()) {
        return tr("Choose an empty folder or an existing installation.");
    }
    QString parent = QFileInfo(clean).absolutePath();
    while (!parent.isEmpty() && !QDir(parent).isRoot()) {
        if (QFile::exists(Manifest::pathIn(parent)))
            return tr("Do not install inside another installation.");
        parent = QFileInfo(parent).absolutePath();
    }
    const qint64 free = availableBytes();
    if (free < 0)
        return tr("The destination drive is not available.");
    if (free >= 0 && quint64(free) < m_required + (m_required * 15 + 99) / 100)
        return tr("This drive has %1 free and %2 is needed.")
            .arg(readable(free), readable(qint64(m_required)));
    if (!elevationNeeded()) {
        QString probe = clean;
        while (!QFileInfo::exists(probe) && !QDir(probe).isRoot())
            probe = QFileInfo(probe).absolutePath();
        QTemporaryFile check(QDir(probe).filePath(QStringLiteral(".htmusic-write-XXXXXX")));
        if (!check.open())
            return tr(
                "This folder is not writable. Choose another location or install for all users.");
    }
    return {};
}

void Session::setScope(Scope scope)
{
    if (m_options.scope == scope || m_installed.present)
        return;
    const QString wasDefault = product::defaultDirectory(m_options.scope);
    m_options.scope = scope;
    if (QDir::cleanPath(m_options.directory) == QDir::cleanPath(wasDefault))
        m_options.directory = product::defaultDirectory(scope);
    Q_EMIT changed();
    Q_EMIT directoryChanged();
}

QString Session::directory() const
{
    return QDir::toNativeSeparators(m_options.directory);
}

Mode Session::maintenanceMode() const
{
    if (!m_installed.present)
        return Mode::Install;
    return m_installed.version == product::version() ? Mode::Repair : Mode::Update;
}

void Session::setDirectory(const QString &directory)
{
    if (m_installed.present)
        return;
    const QString clean = QDir::fromNativeSeparators(directory);
    if (m_options.directory == clean)
        return;
    m_options.directory = clean;
    Q_EMIT directoryChanged();
}

void Session::setComponent(Component component, bool on)
{
    if (m_options.components.testFlag(component) == on)
        return;
    m_options.components.setFlag(component, on);
    Q_EMIT changed();
}

void Session::setDesktopShortcut(bool on)
{
    setComponent(Component::DesktopShortcut, on);
}

void Session::setStartMenuShortcut(bool on)
{
    setComponent(Component::StartMenuShortcut, on);
}

void Session::setLaunchAtSignIn(bool on)
{
    setComponent(Component::LaunchAtSignIn, on);
}

void Session::setUrlScheme(bool on)
{
    setComponent(Component::UrlScheme, on);
}

void Session::setLaunchWhenDone(bool on)
{
    if (m_options.launchWhenDone == on)
        return;
    m_options.launchWhenDone = on;
    Q_EMIT changed();
}

void Session::setRemoveUserData(bool on)
{
    if (m_options.removeUserData == on)
        return;
    m_options.removeUserData = on;
    Q_EMIT changed();
}

void Session::refreshBlockers()
{
    const QString directory = m_installed.present ? m_installed.directory : m_options.directory;
    const QStringList found = directory.isEmpty() ? QStringList() : running::inside(directory);
    if (found == m_blockers)
        return;
    m_blockers = found;
    Q_EMIT blockersChanged();
}

bool Session::closeBlockers()
{
    const QString directory = m_installed.present ? m_installed.directory : m_options.directory;
    const bool closed = directory.isEmpty() || running::askToClose(directory, 8000);
    refreshBlockers();
    return closed;
}

bool Session::elevate()
{
    const QString origin =
        m_options.origin.isEmpty() ? QCoreApplication::applicationFilePath() : m_options.origin;
    const QStringList arguments = commandLine::rebuild(m_options);
    const QString line = arguments.join(QLatin1Char(' '));

    SHELLEXECUTEINFOW info {};
    info.cbSize = sizeof(info);
    info.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
    info.lpVerb = L"runas";
    const QString native = QDir::toNativeSeparators(origin);
    info.lpFile = reinterpret_cast<const wchar_t *>(native.utf16());
    info.lpParameters = reinterpret_cast<const wchar_t *>(line.utf16());
    info.nShow = SW_SHOWNORMAL;

    release();
    if (!ShellExecuteExW(&info)) {
        claim();
        setScope(Scope::CurrentUser);
        m_result = Outcome::ElevationDeclined;
        Q_EMIT elevationDeclined();
        return false;
    }
    if (info.hProcess) {
        if (m_options.silent) {
            QEventLoop loop;
            QTimer timer;
            connect(&timer, &QTimer::timeout, &loop, [&] {
                if (WaitForSingleObject(info.hProcess, 0) == WAIT_OBJECT_0)
                    loop.quit();
            });
            timer.start(100);
            loop.exec();
            DWORD code = DWORD(Outcome::Failed);
            GetExitCodeProcess(info.hProcess, &code);
            m_result = Outcome(code);
        }
        CloseHandle(info.hProcess);
    }
    return true;
}

bool Session::start()
{
    if (m_job.running())
        return false;
    m_options.language = core::Localization::instance().resolved();
    m_complaint = directoryComplaint();
    if (!m_complaint.isEmpty()) {
        m_result = Job::enoughSpace(m_options.directory, m_required) ? Outcome::Failed
                                                                     : Outcome::NotEnoughSpace;
        Q_EMIT changed();
        return false;
    }
    refreshBlockers();
    if (!m_blockers.isEmpty() && m_options.closeApplications)
        closeBlockers();
    if (!m_blockers.isEmpty()) {
        m_complaint = tr("Close the running application and try again.");
        m_result = Outcome::Failed;
        Q_EMIT changed();
        return false;
    }
    if (elevationNeeded()) {
        if (!elevate())
            return false;
        if (!m_options.silent)
            QCoreApplication::quit();
        return true;
    }

    if (m_options.mode == Mode::Uninstall)
        m_job.uninstall(m_options);
    else
        m_job.install(m_options);
    return true;
}

void Session::launchApplication()
{
    const QString target = QDir(m_options.directory).filePath(product::executable());
    if (QFile::exists(target))
        QProcess::startDetached(target, {}, m_options.directory);
}

bool Session::claim()
{
    m_mutex = CreateMutexW(nullptr, FALSE, L"Global\\nekolab.HyperTubeMusic.Setup");
    if (!m_mutex || GetLastError() == ERROR_ALREADY_EXISTS) {
        release();
        m_result = Outcome::AlreadyRunning;
        return false;
    }
    return true;
}

void Session::release()
{
    if (m_mutex)
        CloseHandle(m_mutex);
    m_mutex = nullptr;
}

void Session::setMode(Mode mode)
{
    if (m_options.mode == mode || !m_installed.present)
        return;
    m_options.mode = mode;
    m_required = mode == Mode::Uninstall ? 0 : Job::requiredBytes(m_options);
    Q_EMIT changed();
    Q_EMIT directoryChanged();
}

QString Session::logFile() const
{
    return core::logging::file();
}

QStringList Session::userDataDirectories() const
{
    return {QDir::toNativeSeparators(product::userConfigDirectory()),
            QDir::toNativeSeparators(product::userDataDirectory())};
}

bool Session::downgrade() const
{
    return QVersionNumber::fromString(m_installed.version) > QVersionNumber::fromString(version());
}

void Session::chooseDirectory(const QUrl &url)
{
    if (url.isLocalFile())
        setDirectory(url.toLocalFile());
}

void Session::copyDetails(const QString &message)
{
    QGuiApplication::clipboard()->setText(message + QLatin1Char('\n') + logFile());
}

void Session::finish(int outcome)
{
    QCoreApplication::exit(outcome);
}

QString Session::readable(qint64 bytes) const
{
    return QLocale().formattedDataSize(bytes, 1, QLocale::DataSizeTraditionalFormat);
}

}
