#pragma once

#include "Options.h"

#include <QString>
#include <QStringList>

namespace setup::commandLine {

struct Parsed
{
    Options options;
    bool help = false;
    bool valid = true;
    QString complaint;
};

Parsed parse(const QStringList &arguments);

QString usage();

QStringList rebuild(const Options &options);

}
