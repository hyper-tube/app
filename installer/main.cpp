#include "CommandLine.h"
#include "Job.h"
#include "Options.h"
#include "Product.h"
#include "Session.h"

#include "core/AppInfo.h"
#include "core/Fonts.h"
#include "core/Localization.h"
#include "core/Logging.h"
#include "platform/WindowChrome.h"

#include <QEventLoop>
#include <QDir>
#include <QCursor>
#include <QScreen>
#include <QOperatingSystemVersion>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTextStream>

#include <windows.h>

namespace {

void speak(const QString &text)
{
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        QTextStream stream(stdout);
        stream << text << Qt::endl;
        FreeConsole();
    }
}

int report(setup::Outcome outcome, const QString &complaint)
{
    qCInfo(logPlatform) << "setup outcome" << int(outcome) << complaint;
    if (!complaint.isEmpty())
        speak(complaint);
    return int(outcome);
}

void launchAfter(setup::Session &session, setup::Outcome outcome)
{
    const setup::Options &options = session.options();
    if (options.mode == setup::Mode::Uninstall || !options.launchWhenDone)
        return;
    if (outcome == setup::Outcome::Succeeded || options.launchRequested)
        session.launchApplication();
}

int runHeadless(setup::Session &session)
{
    QEventLoop loop;
    setup::Outcome result = setup::Outcome::Failed;
    QString complaint;

    QObject::connect(session.job(), &setup::Job::finished, &loop,
                     [&](setup::Outcome outcome, const QString &message) {
        result = outcome;
        complaint = message;
        loop.quit();
    });

    if (!session.start() || session.elevationNeeded()) {
        const int status = report(session.result(), session.complaint());
        launchAfter(session, session.result());
        return status;
    }

    loop.exec();

    if (result != setup::Outcome::Succeeded && !complaint.isEmpty())
        speak(complaint);
    launchAfter(session, result);
    return int(result);
}

int runWindowed()
{
    QQmlApplicationEngine engine;
    QObject::connect(&core::Localization::instance(), &core::Localization::resolvedChanged, &engine,
                     &QQmlApplicationEngine::retranslate);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, qApp, [] {
        QCoreApplication::exit(int(setup::Outcome::Failed));
    }, Qt::QueuedConnection);
    engine.loadFromModule("HtMusic.Setup", "Main");
    if (engine.rootObjects().isEmpty())
        return int(setup::Outcome::Failed);

    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
    platform::WindowChrome::instance().bind(window);
    if (QScreen *screen = QGuiApplication::screenAt(QCursor::pos())) {
        const QRect area = screen->availableGeometry();
        window->setPosition(area.center() - QPoint(window->width() / 2, window->height() / 2));
    }

    window->show();
    return QGuiApplication::exec();
}

}

int main(int argc, char *argv[])
{
    core::AppInfo::identify();
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QGuiApplication app(argc, argv);
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/ht-music.svg")));
    core::fonts::install();

    const setup::commandLine::Parsed parsed =
        setup::commandLine::parse(QCoreApplication::arguments().mid(1));

    if (parsed.help) {
        speak(setup::commandLine::usage());
        return int(setup::Outcome::Succeeded);
    }
    if (!parsed.valid) {
        speak(QStringLiteral("setup: ") + parsed.complaint);
        return int(setup::Outcome::BadArguments);
    }

    setup::Options options = parsed.options;
    if (options.logFile.isEmpty())
        options.logFile = QDir::temp().filePath(QStringLiteral("HyperTubeMusic-Setup.log"));
    core::logging::install(options.logFile);
    const auto system = QOperatingSystemVersion::current();
    if (system.majorVersion() < 10
        || (system.majorVersion() == 10 && system.microVersion() < 17763)) {
        return report(
            setup::Outcome::Unsupported,
            QCoreApplication::translate("main", "%1 needs Windows 10 version 1809 or newer.")
                .arg(setup::product::displayName()));
    }
    if (options.origin.isEmpty())
        options.origin = QCoreApplication::applicationFilePath();
    if (!options.language.isEmpty())
        core::Localization::instance().setLanguage(options.language);

    setup::Session &session = setup::Session::instance();
    session.adopt(options);

    if (!session.claim()) {
        return report(setup::Outcome::AlreadyRunning,
                      QCoreApplication::translate("main", "Setup is already running."));
    }
    const int result = options.verySilent ? runHeadless(session) : runWindowed();
    session.release();
    return result;
}
