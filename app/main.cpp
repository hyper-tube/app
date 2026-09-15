#include "control/DownloadActivity.h"
#include "core/AppInfo.h"
#include "core/Fonts.h"
#include "core/Logging.h"
#include "core/Localization.h"
#include "diagnostics/Diagnostics.h"
#include "diagnostics/DiagnosticsSettings.h"
#include "innertube/Session.h"
#include "library/ImageCache.h"
#include "media/ArtworkProvider.h"
#include "media/PlaybackController.h"
#include "media/QueueSync.h"
#include "platform/Desktop.h"
#include "platform/SessionStore.h"
#include "platform/SingleInstance.h"
#include "platform/WindowChrome.h"
#include "platform/WindowState.h"
#include "plugin/PluginRegistry.h"
#include "update/Updater.h"

#include <QApplication>
#include <QIcon>
#include <QProcess>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QtWebEngineQuick>

namespace {

QQuickWindow *shellWindow(const QQmlApplicationEngine &engine)
{
    const QList<QObject *> roots = engine.rootObjects();
    return roots.isEmpty() ? nullptr : qobject_cast<QQuickWindow *>(roots.constFirst());
}

int run(int argc, char *argv[], QStringList &relaunch)
{
    QApplication app(argc, argv);
#ifndef Q_OS_MACOS
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/ht-music.svg")));
#endif
    core::fonts::install();

    platform::SingleInstance running;
    if (!running.claim())
        return 0;

    diagnostics::DiagnosticsSettings &reporting = diagnostics::DiagnosticsSettings::instance();
    reporting.restore();

    const auto &localization = core::Localization::instance();

    library::ImageCache imageCache;
    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("art"), new media::ArtworkProvider(imageCache));

    QObject::connect(&localization, &core::Localization::resolvedChanged, &engine,
                     [&engine] { engine.retranslate(); });

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);

    engine.loadFromModule("HtMusic.App", "Main");
    reporting.bind(engine);

    QQuickWindow *window = shellWindow(engine);
    platform::WindowChrome &chrome = platform::WindowChrome::instance();
    chrome.bind(window);
    platform::WindowState::instance().bind(window);

    auto &playback = media::PlaybackController::instance();
    platform::Desktop const desktop(playback, window);
    QObject::connect(&running, &platform::SingleInstance::raiseRequested, &desktop,
                     &platform::Desktop::raise);

    platform::SessionStore session(playback);
    session.restore();
    media::QueueSync queueSync(playback);
    queueSync.start(session.playedAt());

    control::DownloadActivity const downloadActivity;
    update::Updater::instance().start();

    plugin::PluginRegistry &plugins = plugin::PluginRegistry::instance();
    plugins.restore();
    QObject::connect(&app, &QApplication::aboutToQuit, &plugins,
                     [&plugins] { plugins.shutDown(); });

    const int status = QApplication::exec();
    innertube::Session::instance().finishTrackingRequests();
    if (chrome.relaunching())
        relaunch = QStringList(QCoreApplication::applicationFilePath())
            + QCoreApplication::arguments().mid(1);
    return status;
}

}

int main(int argc, char *argv[])
{
    core::AppInfo::identify();
    core::logging::install();
    diagnostics::guardTermination();

    QQuickStyle::setStyle(QStringLiteral("Basic"));
    QtWebEngineQuick::initialize();

    QStringList relaunch;
    const int status = run(argc, argv, relaunch);
    if (!relaunch.isEmpty())
        QProcess::startDetached(relaunch.constFirst(), relaunch.mid(1));

    diagnostics::shutDown();
    return status;
}
