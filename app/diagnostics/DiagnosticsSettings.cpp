#include "DiagnosticsSettings.h"

#include "Backend.h"
#include "BuildInfo.h"
#include "Diagnostics.h"
#include "LogBridge.h"
#include "core/AppInfo.h"
#include "core/Logging.h"
#include "core/Paths.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSettings>
#include <QSysInfo>
#include <QTimer>

#include <mpv/client.h>

#include <stdexcept>

namespace {

constexpr auto kEnabledKey = "diagnostics/shareReports";
constexpr int kDevelopmentTestDelayMs = 4000;

constexpr QLatin1StringView kProduction("production");

bool production()
{
    return QLatin1StringView(core::build::kDiagnosticsEnvironment) == kProduction;
}

QString developmentVariable(const char *name)
{
    return production() ? QString() : qEnvironmentVariable(name);
}

QString destination()
{
    const QString override = developmentVariable("HT_MUSIC_DIAGNOSTICS_DSN");
    return override.isEmpty() ? QString::fromLatin1(core::build::kDiagnosticsDsn) : override;
}

QString crashHandlerPath()
{
#ifdef Q_OS_WIN
    const QString name = QStringLiteral("crashpad_handler.exe");
#else
    const QString name = QStringLiteral("crashpad_handler");
#endif
    const QDir directory(QCoreApplication::applicationDirPath());
    const QStringList candidates {
        directory.filePath(name),
        directory.filePath(
            QStringLiteral("../libexec/%1/%2").arg(core::AppInfo::identifier(), name)),
    };
    for (const QString &candidate : candidates) {
        if (QFileInfo(candidate).isExecutable())
            return QDir::cleanPath(candidate);
    }
    return {};
}

const char *graphicsApiName(QSGRendererInterface::GraphicsApi api)
{
    switch (api) {
    case QSGRendererInterface::Software: return "software";
    case QSGRendererInterface::OpenVG: return "openvg";
    case QSGRendererInterface::OpenGL: return "opengl";
    case QSGRendererInterface::Direct3D11: return "direct3d11";
    case QSGRendererInterface::Vulkan: return "vulkan";
    case QSGRendererInterface::Metal: return "metal";
    case QSGRendererInterface::Null: return "null";
    case QSGRendererInterface::Direct3D12: return "direct3d12";
    case QSGRendererInterface::Unknown: break;
    }
    return "unknown";
}

diagnostics::Value symbolOf(const char *text)
{
    return diagnostics::Value::symbol(QLatin1StringView(text));
}

void publishBuild()
{
    const unsigned long mpvApi = mpv_client_api_version();

    diagnostics::setTag({"commit", symbolOf(core::build::kCommit)});
    diagnostics::setTag({"arch", diagnostics::Value::symbol(QSysInfo::currentCpuArchitecture())});
    diagnostics::setTag(
        {"build_arch", diagnostics::Value::symbol(QSysInfo::buildCpuArchitecture())});
    diagnostics::setTag({"qt", symbolOf(qVersion())});
    diagnostics::setTag({"mpv_api",
                         diagnostics::Value::symbol(
                             QStringLiteral("%1.%2").arg(mpvApi >> 16U).arg(mpvApi & 0xffffU))});
    diagnostics::setTag({"graphics_api", graphicsApiName(QQuickWindow::graphicsApi())});

    diagnostics::setContext("app",
                            {{"app_name", symbolOf(core::build::kIdentifier)},
                             {"app_version", symbolOf(core::build::kVersion)},
                             {"build_type", symbolOf(core::build::kDiagnosticsEnvironment)},
                             {"commit", symbolOf(core::build::kCommit)}});
    diagnostics::setContext(
        "runtime",
        {{"name", "Qt"}, {"version", symbolOf(qVersion())}, {"build", symbolOf(QT_VERSION_STR)}});
}

void runDevelopmentTest(const QString &test)
{
    if (!diagnostics::active()) {
        qCWarning(logDiagnostics) << "diagnostics test skipped because sharing is off";
        return;
    }

    qCInfo(logDiagnostics) << "running diagnostics test" << test;
    if (test == QLatin1StringView("message"))
        diagnostics::captureMessage("diagnostics.test_message", {{"trigger", "environment"}},
                                    diagnostics::Level::Info);
    else if (test == QLatin1StringView("error"))
        diagnostics::captureError("diagnostics.test_error", {{"trigger", "environment"}});
    else if (test == QLatin1StringView("critical"))
        qCCritical(logDiagnostics) << "deliberate critical message for diagnostics testing";
    else if (test == QLatin1StringView("fatal"))
        qFatal("deliberate fatal message for diagnostics testing");
    else if (test == QLatin1StringView("exception"))
        throw std::runtime_error("deliberate uncaught exception for diagnostics testing");
    else if (test == QLatin1StringView("crash"))
        diagnostics::backend::crash();
    else
        qCWarning(logDiagnostics) << "unknown diagnostics test" << test;
}

}

namespace diagnostics {

DiagnosticsSettings::DiagnosticsSettings(QObject *parent)
    : QObject(parent)
    , m_interactions(m_state)
    , m_enabled(QSettings().value(kEnabledKey, false).toBool())
{
}

DiagnosticsSettings &DiagnosticsSettings::instance()
{
    static auto *settings = new DiagnosticsSettings(QCoreApplication::instance());
    return *settings;
}

DiagnosticsSettings *DiagnosticsSettings::create(QQmlEngine *, QJSEngine *)
{
    QQmlEngine::setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
    return &instance();
}

bool DiagnosticsSettings::available() const
{
    return backend::compiled() && !destination().isEmpty();
}

void DiagnosticsSettings::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;
    m_enabled = enabled;
    QSettings().setValue(kEnabledKey, enabled);
    if (enabled)
        activate();
    else
        deactivate();
    Q_EMIT enabledChanged();
}

void DiagnosticsSettings::restore()
{
    logBridge::install();
    if (m_enabled)
        activate();

    const QString test = developmentVariable("HT_MUSIC_DIAGNOSTICS_TEST");
    if (!test.isEmpty())
        QTimer::singleShot(kDevelopmentTestDelayMs, this, [test] { runDevelopmentTest(test); });
}

void DiagnosticsSettings::bind(QQmlEngine &engine)
{
    m_state.bind(engine);
    if (backend::running())
        m_state.publish();
}

void DiagnosticsSettings::activate()
{
    if (backend::running() || !available())
        return;

    backend::Configuration configuration;
    configuration.dsn = destination();
    configuration.release =
        core::AppInfo::identifier() + QLatin1Char('@') + core::AppInfo::version();
    configuration.environment = QString::fromLatin1(core::build::kDiagnosticsEnvironment);
    configuration.databasePath = core::paths::crashReportDir();
    configuration.handlerPath = crashHandlerPath();
    configuration.verbose = !developmentVariable("HT_MUSIC_DIAGNOSTICS_VERBOSE").isEmpty();

    if (configuration.handlerPath.isEmpty()) {
        qCWarning(logDiagnostics) << "the crash handler is missing, crash reporting stays off";
        return;
    }
    if (!backend::start(configuration)) {
        qCWarning(logDiagnostics) << "crash reporting failed to start";
        return;
    }

    qCInfo(logDiagnostics) << "crash reporting started for" << configuration.environment;
    publishBuild();
    m_state.publish();
    m_interactions.setEnabled(true);
    breadcrumb("diagnostics.started");
}

void DiagnosticsSettings::deactivate()
{
    m_interactions.setEnabled(false);
    if (!backend::running())
        return;
    backend::stop(backend::Ending::Revoked);
    qCInfo(logDiagnostics) << "crash reporting stopped and consent revoked";
}

}
