#include "Job.h"

#include "Archive.h"
#include "Manifest.h"
#include "Product.h"
#include "Registration.h"
#include "Running.h"
#include "SafePath.h"
#include "Transaction.h"

#include "core/Logging.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStorageInfo>
#include <QThread>

#include <fstream>

#include <windows.h>

namespace {

std::filesystem::path toPath(const QString &text)
{
    return std::filesystem::path(text.toStdU16String());
}

QString fromStd(const std::string &text)
{
    return QString::fromStdString(text);
}

bool readFooter(const QString &origin, setup::ArchiveFooter &footer)
{
    return setup::ArchiveFooter::readFrom(toPath(origin), footer);
}

bool removeWithRetries(const QString &path)
{
    for (int attempt = 0; attempt < 40; ++attempt) {
        if (!QFile::exists(path) || QFile::remove(path))
            return true;
        QThread::msleep(100);
    }
    return false;
}

void pruneEmptyFolders(const QString &directory)
{
    QDir root(directory);
    if (!root.exists())
        return;

    const QStringList children =
        root.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
    for (const QString &child : children)
        pruneEmptyFolders(root.filePath(child));

    if (root.entryList(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot)
            .isEmpty())
        QDir().rmdir(directory);
}

}

namespace setup {

Job::Job(QObject *parent)
    : QObject(parent)
{
}

Job::~Job()
{
    cancel();
    if (m_thread) {
        m_thread->quit();
        m_thread->wait();
        delete m_thread;
    }
}

void Job::cancel()
{
    m_cancelled = true;
}

void Job::report(const QString &step)
{
    if (m_step == step)
        return;
    m_step = step;
    Q_EMIT stepChanged();
}

void Job::advance(qreal value)
{
    if (qFuzzyCompare(m_progress, value))
        return;
    m_progress = value;
    Q_EMIT progressChanged();
}

quint64 Job::requiredBytes(const Options &options)
{
    ArchiveFooter footer;
    if (!readFooter(options.origin, footer))
        return 0;

    std::ifstream source(toPath(options.origin), std::ios::binary);
    ArchiveReader reader;
    if (!source || !reader.open(source, footer.payloadOffset, footer.payloadSize))
        return 0;
    return reader.expandedSize() + footer.runtimeOffset + footer.runtimeSize + ArchiveFooter::kSize;
}

bool Job::enoughSpace(const QString &directory, quint64 needed)
{
    QString probe = directory;
    while (!probe.isEmpty() && !QFileInfo::exists(probe)) {
        const QString parent = QFileInfo(probe).absolutePath();
        if (parent == probe)
            break;
        probe = parent;
    }
    const QStorageInfo storage(probe);
    if (!storage.isValid())
        return false;
    return storage.isReady() && storage.bytesAvailable() >= 0
        && quint64(storage.bytesAvailable()) >= needed + (needed * 15 + 99) / 100;
}

void Job::install(const Options &options)
{
    run(options, false);
}

void Job::uninstall(const Options &options)
{
    run(options, true);
}

void Job::run(const Options &options, bool removing)
{
    if (m_running)
        return;

    if (m_thread) {
        m_thread->wait();
        delete m_thread;
        m_thread = nullptr;
    }
    m_cancelled = false;
    m_running = true;
    m_progress = 0;
    Q_EMIT runningChanged();
    Q_EMIT progressChanged();

    m_thread = QThread::create([this, options, removing] {
        const HRESULT apartment = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        QString message;
        const bool ok =
            removing ? performUninstall(options, message) : performInstall(options, message);
        if (SUCCEEDED(apartment))
            CoUninitialize();
        Outcome outcome = Outcome::Succeeded;
        if (!ok)
            outcome = m_cancelled ? Outcome::Cancelled : Outcome::Failed;

        qCInfo(logPlatform) << "setup outcome" << int(outcome) << message;
        QMetaObject::invokeMethod(this, [this, outcome, message] {
            m_running = false;
            Q_EMIT runningChanged();
            Q_EMIT finished(outcome, message);
        }, Qt::QueuedConnection);
    });
    m_thread->start();
}

bool Job::performInstall(const Options &options, QString &message)
{
    ArchiveFooter footer;
    if (!readFooter(options.origin, footer) || footer.payloadSize == 0) {
        message = tr("This setup file is damaged.");
        return false;
    }

    std::ifstream source(toPath(options.origin), std::ios::binary);
    if (!source) {
        message = tr("The setup file could not be opened.");
        return false;
    }

    QMetaObject::invokeMethod(this, [this] { report(tr("Checking the setup file")); },
                              Qt::QueuedConnection);
    if (!verifyRegion(source, footer.payloadOffset, footer.payloadSize, footer.payloadDigest)) {
        message = tr("This setup file is damaged. Download it again.");
        return false;
    }

    ArchiveReader reader;
    if (!reader.open(source, footer.payloadOffset, footer.payloadSize)) {
        message = fromStd(reader.error());
        return false;
    }

    const QString directory = QDir::cleanPath(options.directory);
    if (!enoughSpace(directory, requiredBytes(options))) {
        message = tr("There is not enough free space on this drive.");
        return false;
    }
    if (!running::inside(directory).isEmpty()) {
        message = tr("Close the running application and try again.");
        return false;
    }
    Transaction transaction(directory);
    if (!transaction.prepare()) {
        message = tr("Choose an empty writable folder or an existing installation.");
        return false;
    }

    QMetaObject::invokeMethod(this, [this] { report(tr("Copying files")); }, Qt::QueuedConnection);

    const bool extracted =
        reader.extract(toPath(transaction.staging()),
                       [this](const std::string &path, std::uint64_t done, std::uint64_t total) {
        if (m_cancelled)
            return false;
        const qreal share = total > 0 ? qreal(done) / qreal(total) : 0.0;
        const QString name = QFileInfo(QString::fromStdString(path)).fileName();
        QMetaObject::invokeMethod(this, [this, share, name] {
            advance(share * 0.92);
            report(name);
        }, Qt::QueuedConnection);
        return true;
    });

    if (!extracted) {
        message = m_cancelled ? tr("Installation was cancelled.") : fromStd(reader.error());
        return false;
    }

    QMetaObject::invokeMethod(this, [this] {
        advance(0.94);
        report(tr("Registering the application"));
    }, Qt::QueuedConnection);

    Manifest manifest;
    manifest.version = product::version();
    manifest.language = options.language;
    manifest.scope = options.scope;
    manifest.components = options.components;
    manifest.files.reserve(int(reader.written().size()));
    for (const ArchiveEntry &entry : reader.entries()) {
        const QString path = QString::fromStdString(entry.path);
        manifest.files.append(path);
        manifest.sizes.insert(path, entry.size);
        manifest.hashes.insert(path, QString::fromStdString(hex(entry.digest)));
    }

    const QString remover = QDir(transaction.staging()).filePath(product::uninstaller());
    if (!cloneWithoutPayload(toPath(options.origin), toPath(remover))) {
        message = tr("The uninstaller could not be written.");
        return false;
    }
    manifest.files.append(product::uninstaller());
    Digest removerDigest;
    if (!digestOf(toPath(remover), removerDigest)) {
        message = tr("The uninstaller could not be written.");
        return false;
    }
    manifest.sizes.insert(product::uninstaller(), QFileInfo(remover).size());
    manifest.hashes.insert(product::uninstaller(), QString::fromStdString(hex(removerDigest)));

    if (!manifest.write(transaction.staging())) {
        message = tr("The installation record could not be written.");
        return false;
    }

    if (m_cancelled) {
        message = tr("Installation was cancelled.");
        return false;
    }
    if (!transaction.publish()) {
        message = tr("The installation could not be replaced. Close applications using this folder "
                     "and try again.");
        return false;
    }
    const auto restoreRegistration = [&] {
        registration::removeShortcuts(options.scope);
        registration::unregisterUrlScheme(options.scope);
        registration::erase(options.scope);
        const Manifest &previous = transaction.previous();
        if (previous.valid) {
            Options restored = options;
            restored.scope = previous.scope;
            restored.components = previous.components;
            restored.language = previous.language;
            registration::write(restored, directory, 0, previous.version);
            registration::createShortcuts(restored, directory);
            registration::setLaunchAtSignIn(restored.components.testFlag(Component::LaunchAtSignIn),
                                            restored.scope, directory);
            if (restored.components.testFlag(Component::UrlScheme))
                registration::registerUrlScheme(restored.scope, directory);
        }
    };
    if (!registration::write(options, directory, requiredBytes(options))) {
        restoreRegistration();
        message = tr("The application could not be registered with Windows.");
        return false;
    }

    QMetaObject::invokeMethod(this, [this] {
        advance(0.97);
        report(tr("Creating shortcuts"));
    }, Qt::QueuedConnection);

    registration::removeShortcuts(options.scope);
    registration::unregisterUrlScheme(options.scope);
    const bool integrated = registration::createShortcuts(options, directory)
        && registration::setLaunchAtSignIn(options.components.testFlag(Component::LaunchAtSignIn),
                                           options.scope, directory)
        && (!options.components.testFlag(Component::UrlScheme)
            || registration::registerUrlScheme(options.scope, directory));
    if (!integrated) {
        restoreRegistration();
        message = tr("Windows shortcuts or startup settings could not be updated.");
        return false;
    }
    if (!transaction.commit())
        qCWarning(logPlatform) << "The previous installation could not be fully removed";

    QMetaObject::invokeMethod(this, [this] {
        advance(1.0);
        report(tr("Finished"));
    }, Qt::QueuedConnection);
    return true;
}

bool Job::performUninstall(const Options &options, QString &message)
{
    const QString directory = QDir::cleanPath(options.directory);
    const Manifest manifest = Manifest::read(directory);
    if (!manifest.valid || !safeDestination(toPath(directory))) {
        message =
            tr("The installation record is missing or damaged. Run setup to repair it first.");
        return false;
    }
    if (!running::inside(directory).isEmpty()) {
        message = tr("Close the running application and try again.");
        return false;
    }
    for (const QString &relative : manifest.files) {
        if (!safeDestination(toPath(QDir(directory).filePath(relative)))) {
            message = tr(
                "The installation contains a redirected folder. Restore it before uninstalling.");
            return false;
        }
    }

    QMetaObject::invokeMethod(this, [this] { report(tr("Removing files")); }, Qt::QueuedConnection);

    const int total = qMax(1, manifest.files.size());
    int done = 0;
    for (const QString &relative : manifest.files) {
        if (relative.compare(product::uninstaller(), Qt::CaseInsensitive) == 0)
            continue;
        if (!removeWithRetries(QDir(directory).filePath(relative))) {
            message =
                tr("A file could not be removed: %1. Close applications using it and try again.")
                    .arg(relative);
            return false;
        }
        ++done;
        if ((done % 64) == 0 || done == total) {
            const qreal share = qreal(done) / qreal(total);
            QMetaObject::invokeMethod(this, [this, share] { advance(share * 0.8); },
                                      Qt::QueuedConnection);
        }
    }

    if (!removeWithRetries(QDir(directory).filePath(product::uninstaller()))
        || !removeWithRetries(Manifest::pathIn(directory))) {
        message = tr("The uninstaller could not finish removing its files. Try again.");
        return false;
    }

    QMetaObject::invokeMethod(this, [this] {
        advance(0.9);
        report(tr("Removing shortcuts"));
    }, Qt::QueuedConnection);

    registration::removeShortcuts(options.scope);
    registration::unregisterUrlScheme(options.scope);
    registration::erase(options.scope);
    pruneEmptyFolders(directory);

    if (options.removeUserData) {
        QMetaObject::invokeMethod(this, [this] { report(tr("Removing your data")); },
                                  Qt::QueuedConnection);
        if (!QDir(product::userConfigDirectory()).removeRecursively()
            || !QDir(product::userDataDirectory()).removeRecursively()) {
            message = tr("Some user data could not be removed.");
            return false;
        }
    }

    QMetaObject::invokeMethod(this, [this] {
        advance(1.0);
        report(tr("Finished"));
    }, Qt::QueuedConnection);
    return true;
}

}
