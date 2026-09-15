#include "OfflineSearchModel.h"

#include "library/Downloads.h"
#include "library/OfflineSearch.h"

namespace {

model::Item searchSource(const QString &query)
{
    model::Item item;
    item.title = query;
    item.kind = QStringLiteral("search");
    return item;
}

}

namespace model {

OfflineSearchModel::OfflineSearchModel(const QString &query, QObject *parent)
    : BrowseModel(searchSource(query), QStringLiteral("search"), parent)
{
    connect(&library::Downloads::instance(), &library::Downloads::catalogueChanged, this,
            &OfflineSearchModel::reload);
}

void OfflineSearchModel::reload()
{
    replaceTracks(library::offlineSearch::matches(title()));
}

}
