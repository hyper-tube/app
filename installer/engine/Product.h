#pragma once

#include "Options.h"

#include <QString>

namespace setup::product {

QString displayName();
QString version();
QString publisher();
QString executable();
QString uninstaller();
QString folderName();
QString appUserModelId();
QString websiteUrl();
QString repositoryUrl();
QString urlScheme();

QString defaultDirectory(Scope scope);
QString startMenuDirectory(Scope scope);
QString desktopDirectory(Scope scope);
QString uninstallKey(Scope scope);
QString publisherKey(Scope scope);
QString settingsKey(Scope scope);
QString appPathsKey(Scope scope);
QString runKey(Scope scope);

QString userConfigDirectory();
QString userDataDirectory();

}
