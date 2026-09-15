#pragma once

#include "BrowseModel.h"

namespace model {

class OfflineSearchModel : public BrowseModel
{
    Q_OBJECT

public:
    OfflineSearchModel(const QString &query, QObject *parent);

    bool local() const override { return true; }
    void reload() override;
};

}
