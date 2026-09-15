#include "Transaction.h"

#include "SafePath.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QUuid>

namespace setup {

Transaction::Transaction(const QString &directory)
    : m_directory(QDir::cleanPath(directory))
    , m_staging(m_directory + QStringLiteral(".setup-")
                + QUuid::createUuid().toString(QUuid::Id128))
    , m_backup(m_staging + QStringLiteral(".previous"))
    , m_previous(Manifest::read(m_directory))
{
}

Transaction::~Transaction()
{
    if (!m_committed && m_published)
        QDir(m_directory).removeRecursively();
    if (!m_committed && m_backedUp)
        QDir().rename(m_backup, m_directory);
    if (m_prepared)
        QDir(m_staging).removeRecursively();
}

bool Transaction::prepare()
{
    if (!safeDestination(std::filesystem::path(m_directory.toStdU16String())))
        return false;
    const QDir destination(m_directory);
    if (destination.exists() && !m_previous.valid
        && !destination
                .entryList(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot)
                .isEmpty()) {
        return false;
    }
    m_prepared = QDir().mkpath(m_staging);
    return m_prepared;
}

bool Transaction::publish()
{
    QSet<QString> owned;
    for (const QString &file : m_previous.files)
        owned.insert(file.toCaseFolded());
    owned.insert(QStringLiteral("install.json"));

    QDirIterator iterator(m_directory,
                          QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot,
                          QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        const QString file = iterator.next();
        if (!safeDestination(std::filesystem::path(file.toStdU16String())))
            return false;
        if (QFileInfo(file).isDir())
            continue;
        const QString relative = QDir(m_directory).relativeFilePath(file);
        if (owned.contains(relative.toCaseFolded()))
            continue;
        const QString target = QDir(m_staging).filePath(relative);
        if (!QDir().mkpath(QFileInfo(target).absolutePath()) || !QFile::copy(file, target))
            return false;
    }

    if (QDir(m_directory).exists()) {
        if (!QDir().rename(m_directory, m_backup))
            return false;
        m_backedUp = true;
    }
    if (!QDir().rename(m_staging, m_directory))
        return false;
    m_published = true;
    return true;
}

bool Transaction::commit()
{
    m_committed = true;
    return !m_backedUp || QDir(m_backup).removeRecursively();
}

}
