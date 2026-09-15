#include "ColorFile.h"

#include <QDir>
#include <QFileInfo>

namespace theme {

ColorFile::ColorFile(const QString &id, const QString &name, const QString &path, QObject *parent)
    : ColorSource(id, name, path, parent)
{
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &ColorFile::refresh);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &ColorFile::refresh);
}

void ColorFile::refresh()
{
    rewatch();

    if (!QFileInfo::exists(location())) {
        clear();
        return;
    }
    read(location());
}

void ColorFile::rewatch()
{
    const QString directory = QFileInfo(location()).absolutePath();
    if (QDir(directory).exists() && !m_watcher.directories().contains(directory))
        m_watcher.addPath(directory);

    if (QFileInfo::exists(location()) && !m_watcher.files().contains(location()))
        m_watcher.addPath(location());
}

}
