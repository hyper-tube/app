#pragma once

#include "ColorSource.h"

namespace theme {

class ThemeColors : public ColorSource
{
    Q_OBJECT

public:
    explicit ThemeColors(QObject *parent);

    static QString sourceId();

    void refresh() override;
};

}
