#pragma once

#include "BrowseModel.h"

namespace model {

class DownloadsModel : public BrowseModel
{
    Q_OBJECT

public:
    explicit DownloadsModel(QObject *parent = nullptr);

    bool local() const override { return true; }
    bool searchable() const override { return true; }
    void reload() override;
};

}
