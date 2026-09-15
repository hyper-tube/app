#pragma once

#include "ColorFile.h"

namespace theme {

class TerminalColors : public ColorFile
{
    Q_OBJECT

public:
    TerminalColors(const QString &id, const QString &name, const QString &path, QObject *parent);

protected:
    void read(const QString &path) override;
};

}
