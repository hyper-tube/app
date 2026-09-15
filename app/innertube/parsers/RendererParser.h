#pragma once

#include "model/Chip.h"
#include "model/Shelf.h"
#include "model/SortOption.h"

#include <QJsonObject>

namespace innertube::parsers {

struct Page
{
    model::Item header;
    QList<model::Shelf> shelves;
    QList<model::SortOption> sortOptions;
    QList<model::Chip> chips;
    QList<model::Chip> scopes;
    QString continuation;
};

class RendererParser
{
public:
    static Page parse(const QJsonObject &response);
};

}
