#pragma once

#include "Manifest.h"

#include <QString>

namespace setup {

class Transaction
{
public:
    explicit Transaction(const QString &directory);
    ~Transaction();

    bool prepare();
    bool publish();
    bool commit();
    const QString &staging() const { return m_staging; }
    const Manifest &previous() const { return m_previous; }

private:
    QString m_directory;
    QString m_staging;
    QString m_backup;
    Manifest m_previous;
    bool m_prepared = false;
    bool m_backedUp = false;
    bool m_published = false;
    bool m_committed = false;
};

}
