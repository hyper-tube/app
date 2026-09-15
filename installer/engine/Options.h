#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QtQmlIntegration>

namespace setup {

Q_NAMESPACE
QML_NAMED_ELEMENT(Setup)

enum class Scope {
    CurrentUser,
    AllUsers,
};
Q_ENUM_NS(Scope)

enum class Mode {
    Install,
    Update,
    Repair,
    Uninstall,
};
Q_ENUM_NS(Mode)

enum class Component {
    DesktopShortcut = 0x01,
    StartMenuShortcut = 0x02,
    LaunchAtSignIn = 0x04,
    UrlScheme = 0x08,
};
Q_ENUM_NS(Component)
Q_DECLARE_FLAGS(Components, Component)
Q_DECLARE_OPERATORS_FOR_FLAGS(Components)

enum class Outcome {
    Succeeded = 0,
    Failed = 1,
    Cancelled = 2,
    ElevationDeclined = 3,
    AlreadyRunning = 4,
    Unsupported = 5,
    NotEnoughSpace = 6,
    BadArguments = 7,
};
Q_ENUM_NS(Outcome)

struct Options
{
    Mode mode = Mode::Install;
    Scope scope = Scope::CurrentUser;
    QString directory;
    Components components = Components(Component::DesktopShortcut) | Component::StartMenuShortcut;
    QString language;
    QString origin;
    QString logFile;
    bool silent = false;
    bool verySilent = false;
    bool launchWhenDone = true;
    bool launchRequested = false;
    bool closeApplications = false;
    bool removeUserData = false;
    bool elevated = false;
    bool resume = false;
    bool noRestart = false;
    bool scopeExplicit = false;
    bool componentsExplicit = false;
};

}
