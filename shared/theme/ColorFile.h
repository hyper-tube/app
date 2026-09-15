#pragma once

#include "ColorSource.h"

#include <QFileSystemWatcher>

namespace theme {

class ColorFile : public ColorSource
{
    Q_OBJECT

public:
    ColorFile(const QString &id, const QString &name, const QString &path, QObject *parent);

    void refresh() final;

protected:
    virtual void read(const QString &path) = 0;

private:
    void rewatch();

    QFileSystemWatcher m_watcher;
};

}
