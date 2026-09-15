#include "DownloadsModel.h"

#include "library/Downloads.h"
#include "library/OfflineSearch.h"

namespace {

model::Item downloadsSource()
{
    model::Item item;
    item.title = QStringLiteral("Downloads");
    item.kind = QStringLiteral("downloads");
    return item;
}

}

namespace model {

DownloadsModel::DownloadsModel(QObject *parent)
    : BrowseModel(downloadsSource(), {}, parent)
{
    connect(&library::Downloads::instance(), &library::Downloads::catalogueChanged, this,
            &DownloadsModel::reload);
}

void DownloadsModel::reload()
{
    replaceTracks(filter().isEmpty() ? library::Downloads::instance().tracks()
                                     : library::offlineSearch::matches(filter()));
}

}
