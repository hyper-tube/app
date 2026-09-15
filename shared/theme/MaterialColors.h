#pragma once

#include "ColorFile.h"

namespace theme {

class MaterialColors : public ColorFile
{
    Q_OBJECT

public:
    MaterialColors(const QString &id, const QString &name, const QString &path, QObject *parent);

protected:
    void read(const QString &path) override;
};

}
