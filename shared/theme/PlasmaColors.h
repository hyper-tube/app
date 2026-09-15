#pragma once

#include "ColorFile.h"

namespace theme {

class PlasmaColors : public ColorFile
{
    Q_OBJECT

public:
    PlasmaColors(const QString &path, QObject *parent);

protected:
    void read(const QString &path) override;
};

}
