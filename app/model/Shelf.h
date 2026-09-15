#pragma once

#include "Item.h"

#include <QList>
#include <QString>

namespace model {

struct Shelf
{
    QString title;
    QString subtitle;
    QList<Item> items;
    QString continuation;
    QString description;
    QString message;
    Item more;
    Item lead;
    bool horizontal = false;
    bool card = false;
    bool navigation = false;
    bool ranked = false;
};

}
