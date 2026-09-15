#pragma once

#include "Options.h"

#include <QString>

namespace setup {

struct Installed
{
    bool present = false;
    Scope scope = Scope::CurrentUser;
    QString version;
    QString directory;
    Components components;
    QString language;
};

namespace registration {

bool write(const Options &options, const QString &directory, quint64 installedBytes,
           const QString &version = {});
void erase(Scope scope);

Installed find();
Installed find(Scope scope);

bool createShortcuts(const Options &options, const QString &directory);
void removeShortcuts(Scope scope);

bool setLaunchAtSignIn(bool enabled, Scope scope, const QString &directory);
bool registerUrlScheme(Scope scope, const QString &directory);
void unregisterUrlScheme(Scope scope);

}

}
