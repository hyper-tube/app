#pragma once

#include "ColorSource.h"

namespace theme {

class PlatformColors : public ColorSource
{
    Q_OBJECT

public:
    explicit PlatformColors(QObject *parent);

    void refresh() override;

private:
    void observe();
};

}
