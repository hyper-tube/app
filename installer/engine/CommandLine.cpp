#include "CommandLine.h"

#include <QDir>

namespace {

const QStringList kLanguages {QStringLiteral("en"), QStringLiteral("ru"), QStringLiteral("uk")};

QString unquote(QString value)
{
    if (value.size() >= 2 && value.startsWith(QLatin1Char('"')) && value.endsWith(QLatin1Char('"')))
        return value.mid(1, value.size() - 2);
    return value;
}

bool matches(const QString &argument, const QString &name)
{
    return argument.compare(name, Qt::CaseInsensitive) == 0;
}

bool takesValue(const QString &argument, const QString &name, QString &value)
{
    if (!argument.startsWith(name, Qt::CaseInsensitive))
        return false;
    if (argument.size() == name.size())
        return false;
    if (argument.at(name.size()) != QLatin1Char('='))
        return false;
    value = unquote(argument.mid(name.size() + 1));
    return true;
}

setup::Component componentFor(const QString &name)
{
    if (name == QLatin1String("desktopicon"))
        return setup::Component::DesktopShortcut;
    if (name == QLatin1String("startmenu"))
        return setup::Component::StartMenuShortcut;
    if (name == QLatin1String("autostart"))
        return setup::Component::LaunchAtSignIn;
    return setup::Component::UrlScheme;
}

bool knownComponent(const QString &name)
{
    return name == QLatin1String("desktopicon") || name == QLatin1String("startmenu")
        || name == QLatin1String("autostart") || name == QLatin1String("protocol");
}

}

namespace setup::commandLine {

Parsed parse(const QStringList &arguments)
{
    Parsed parsed;
    Options &options = parsed.options;

    for (int index = 0; index < arguments.size(); ++index) {
        const QString argument = arguments.at(index);
        QString value;

        if (matches(argument, QStringLiteral("/HELP")) || matches(argument, QStringLiteral("/?"))
            || matches(argument, QStringLiteral("--help"))) {
            parsed.help = true;
        } else if (matches(argument, QStringLiteral("--origin"))) {
            if (index + 1 < arguments.size())
                options.origin = arguments.at(++index);
            else {
                parsed.valid = false;
                parsed.complaint = QStringLiteral("--origin needs a path");
            }
        } else if (matches(argument, QStringLiteral("--elevated"))) {
            options.resume = true;
        } else if (matches(argument, QStringLiteral("/SILENT"))
                   || matches(argument, QStringLiteral("/S"))) {
            options.silent = true;
        } else if (matches(argument, QStringLiteral("/VERYSILENT"))) {
            options.silent = true;
            options.verySilent = true;
        } else if (matches(argument, QStringLiteral("/SUPPRESSMSGBOXES"))) {
            options.verySilent = true;
        } else if (matches(argument, QStringLiteral("/CURRENTUSER"))) {
            options.scope = Scope::CurrentUser;
            options.scopeExplicit = true;
        } else if (matches(argument, QStringLiteral("/ALLUSERS"))) {
            options.scope = Scope::AllUsers;
            options.scopeExplicit = true;
        } else if (matches(argument, QStringLiteral("/NOICONS"))) {
            options.componentsExplicit = true;
            options.components &= ~Components(Component::DesktopShortcut);
            options.components &= ~Components(Component::StartMenuShortcut);
        } else if (matches(argument, QStringLiteral("/NOLAUNCH"))) {
            options.launchWhenDone = false;
        } else if (matches(argument, QStringLiteral("/LAUNCH"))) {
            options.launchRequested = true;
        } else if (matches(argument, QStringLiteral("/CLOSEAPPLICATIONS"))) {
            options.closeApplications = true;
        } else if (matches(argument, QStringLiteral("/NORESTART"))) {
            options.noRestart = true;
        } else if (matches(argument, QStringLiteral("/UNINSTALL"))) {
            options.mode = Mode::Uninstall;
        } else if (matches(argument, QStringLiteral("/REPAIR"))) {
            options.mode = Mode::Repair;
        } else if (matches(argument, QStringLiteral("/UPDATE"))) {
            options.mode = Mode::Update;
        } else if (matches(argument, QStringLiteral("/REMOVEDATA"))) {
            options.removeUserData = true;
        } else if (takesValue(argument, QStringLiteral("/DIR"), value)
                   || takesValue(argument, QStringLiteral("/D"), value)) {
            options.directory = QDir::fromNativeSeparators(value);
            if (value.isEmpty()) {
                parsed.valid = false;
                parsed.complaint = QStringLiteral("the destination cannot be empty");
            }
        } else if (takesValue(argument, QStringLiteral("/LANG"), value)) {
            if (!kLanguages.contains(value.toLower())) {
                parsed.valid = false;
                parsed.complaint = QStringLiteral("unknown language: ") + value;
            }
            options.language = value.toLower();
        } else if (takesValue(argument, QStringLiteral("/LOG"), value)) {
            options.logFile = QDir::fromNativeSeparators(value);
        } else if (takesValue(argument, QStringLiteral("/COMPONENTS"), value)) {
            options.componentsExplicit = true;
            options.components = Components();
            const QStringList names = value.split(QLatin1Char(','), Qt::SkipEmptyParts);
            for (const QString &name : names) {
                const QString trimmed = name.trimmed().toLower();
                if (!knownComponent(trimmed)) {
                    parsed.valid = false;
                    parsed.complaint = QStringLiteral("unknown component: ") + trimmed;
                    continue;
                }
                options.components |= componentFor(trimmed);
            }
        } else {
            parsed.valid = false;
            parsed.complaint = QStringLiteral("unknown option: ") + argument;
        }
    }

    if (!options.scopeExplicit && options.mode == Mode::Install)
        options.scope = Scope::CurrentUser;

    if (options.silent && !options.launchRequested)
        options.launchWhenDone = false;
    return parsed;
}

QString usage()
{
    return QStringLiteral(
        "Usage: setup [options]\n"
        "\n"
        "  /SILENT              Show progress only\n"
        "  /VERYSILENT          Show nothing\n"
        "  /DIR=\"path\"          Install into this folder\n"
        "  /CURRENTUSER         Install for the current user (default)\n"
        "  /ALLUSERS            Install for every user, needs administrator rights\n"
        "  /COMPONENTS=\"a,b\"    desktopicon, startmenu, autostart, protocol\n"
        "  /NOICONS             Create no shortcuts\n"
        "  /LANG=en|ru|uk       Interface language\n"
        "  /LOG=\"path\"          Write the log here\n"
        "  /NOLAUNCH            Do not start the application afterwards\n"
        "  /LAUNCH              Start the application when setup ends, even silently\n"
        "                       and even when setup did not succeed\n"
        "  /CLOSEAPPLICATIONS   Close a running copy of the application instead of stopping\n"
        "  /UPDATE /REPAIR /UNINSTALL   Maintenance modes\n"
        "  /REMOVEDATA          With /UNINSTALL, also delete settings and downloads\n"
        "  /HELP                Show this text\n");
}

QStringList rebuild(const Options &options)
{
    QStringList arguments;
    arguments << QStringLiteral("--elevated");
    arguments << (options.scope == Scope::AllUsers ? QStringLiteral("/ALLUSERS")
                                                   : QStringLiteral("/CURRENTUSER"));
    if (!options.directory.isEmpty())
        arguments << QStringLiteral("/DIR=\"%1\"").arg(QDir::toNativeSeparators(options.directory));
    if (!options.language.isEmpty())
        arguments << QStringLiteral("/LANG=%1").arg(options.language);
    if (options.verySilent)
        arguments << QStringLiteral("/VERYSILENT");
    else if (options.silent)
        arguments << QStringLiteral("/SILENT");
    if (!options.launchWhenDone)
        arguments << QStringLiteral("/NOLAUNCH");
    if (options.noRestart)
        arguments << QStringLiteral("/NORESTART");
    if (options.closeApplications)
        arguments << QStringLiteral("/CLOSEAPPLICATIONS");
    if (!options.logFile.isEmpty())
        arguments << QStringLiteral("/LOG=\"%1\"").arg(QDir::toNativeSeparators(options.logFile));
    if (options.mode == Mode::Update)
        arguments << QStringLiteral("/UPDATE");
    if (options.mode == Mode::Repair)
        arguments << QStringLiteral("/REPAIR");
    if (options.mode == Mode::Uninstall)
        arguments << QStringLiteral("/UNINSTALL");
    if (options.removeUserData)
        arguments << QStringLiteral("/REMOVEDATA");

    QStringList names;
    if (options.components.testFlag(Component::DesktopShortcut))
        names << QStringLiteral("desktopicon");
    if (options.components.testFlag(Component::StartMenuShortcut))
        names << QStringLiteral("startmenu");
    if (options.components.testFlag(Component::LaunchAtSignIn))
        names << QStringLiteral("autostart");
    if (options.components.testFlag(Component::UrlScheme))
        names << QStringLiteral("protocol");
    arguments << QStringLiteral("/COMPONENTS=\"%1\"").arg(names.join(QLatin1Char(',')));

    return arguments;
}

}
