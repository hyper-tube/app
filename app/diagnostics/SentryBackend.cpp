#include "Backend.h"

#include <QByteArrayView>
#include <QFile>

#include <sentry.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <variant>

namespace {

constexpr int kMaximumBreadcrumbs = 100;
constexpr quint64 kShutdownTimeoutMs = 2000;

constexpr std::array kPathKeys {QByteArrayView("code_file"), QByteArrayView("debug_file")};
constexpr std::array kIdentifyingKeys {"request", "server_name"};

std::atomic<bool> started = false;

sentry_level_t sentryLevel(diagnostics::Level level)
{
    switch (level) {
    case diagnostics::Level::Debug: return SENTRY_LEVEL_DEBUG;
    case diagnostics::Level::Info: return SENTRY_LEVEL_INFO;
    case diagnostics::Level::Warning: return SENTRY_LEVEL_WARNING;
    case diagnostics::Level::Error: return SENTRY_LEVEL_ERROR;
    case diagnostics::Level::Fatal: break;
    }
    return SENTRY_LEVEL_FATAL;
}

const char *levelName(diagnostics::Level level)
{
    switch (level) {
    case diagnostics::Level::Debug: return "debug";
    case diagnostics::Level::Info: return "info";
    case diagnostics::Level::Warning: return "warning";
    case diagnostics::Level::Error: return "error";
    case diagnostics::Level::Fatal: break;
    }
    return "fatal";
}

const char *breadcrumbType(QByteArrayView category, diagnostics::Level level)
{
    if (category.startsWith("navigation"))
        return "navigation";
    if (category.startsWith("ui."))
        return "user";
    if (level == diagnostics::Level::Error || level == diagnostics::Level::Fatal)
        return "error";
    return "default";
}

sentry_value_t sentryValue(const diagnostics::Value &value)
{
    const diagnostics::Value::Data &data = value.data();
    if (const auto *flag = std::get_if<bool>(&data))
        return sentry_value_new_bool(*flag ? 1 : 0);
    if (const auto *integer = std::get_if<qint64>(&data))
        return sentry_value_new_int64(*integer);
    if (const auto *real = std::get_if<double>(&data))
        return sentry_value_new_double(*real);
    const QByteArray &text = std::get<QByteArray>(data);
    return sentry_value_new_string_n(text.constData(), size_t(text.size()));
}

sentry_value_t sentryObject(std::span<const diagnostics::Field> fields)
{
    sentry_value_t object = sentry_value_new_object();
    for (const diagnostics::Field &field : fields)
        sentry_value_set_by_key(object, field.key, sentryValue(field.value));
    return object;
}

QByteArray summaryOf(std::span<const diagnostics::Field> fields)
{
    QByteArray summary;
    for (const diagnostics::Field &field : fields) {
        if (!summary.isEmpty())
            summary += ' ';
        summary += field.key;
        summary += '=';
        summary += field.value.text();
    }
    return summary;
}

QByteArrayView baseName(QByteArrayView path)
{
    const qsizetype separator = qMax(path.lastIndexOf('/'), path.lastIndexOf('\\'));
    return path.sliced(separator + 1);
}

int copyImageEntry(const char *key, sentry_value_t value, void *target)
{
    const sentry_value_t image = *static_cast<sentry_value_t *>(target);
    const QByteArrayView name(key);
    if (std::ranges::find(kPathKeys, name) != kPathKeys.end()) {
        const QByteArrayView file = baseName(QByteArrayView(sentry_value_as_string(value)));
        sentry_value_set_by_key(image, key,
                                sentry_value_new_string_n(file.data(), size_t(file.size())));
        return 0;
    }
    sentry_value_incref(value);
    sentry_value_set_by_key(image, key, value);
    return 0;
}

int copyImage(sentry_value_t image, void *target)
{
    sentry_value_t copy = sentry_value_new_object();
    sentry_value_foreach_key_value(image, &copyImageEntry, &copy);
    sentry_value_append(*static_cast<sentry_value_t *>(target), copy);
    return 0;
}

sentry_value_t withoutLocalPaths(sentry_value_t event, void *, void *)
{
    for (const char *key : kIdentifyingKeys)
        sentry_value_remove_by_key(event, key);

    const bool symbolicated = !sentry_value_is_null(sentry_value_get_by_key(event, "exception"))
        || !sentry_value_is_null(sentry_value_get_by_key(event, "threads"));
    if (!symbolicated) {
        sentry_value_remove_by_key(event, "debug_meta");
        return event;
    }

    const sentry_value_t images =
        sentry_value_get_by_key(sentry_value_get_by_key(event, "debug_meta"), "images");
    if (sentry_value_get_length(images) == 0)
        return event;

    sentry_value_t sanitized = sentry_value_new_list();
    sentry_value_foreach_value(images, &copyImage, &sanitized);
    const sentry_value_t debugMeta = sentry_value_new_object();
    sentry_value_set_by_key(debugMeta, "images", sanitized);
    sentry_value_set_by_key(event, "debug_meta", debugMeta);
    return event;
}

sentry_value_t exceptionEvent(const QByteArray &type, const QByteArray &description)
{
    sentry_value_t event = sentry_value_new_event();
    const sentry_value_t exception = sentry_value_new_exception_n(
        type.constData(), size_t(type.size()), description.constData(), size_t(description.size()));
    sentry_value_set_stacktrace(exception, nullptr, 0);
    sentry_event_add_exception(event, exception);
    return event;
}

sentry_value_t messageEvent(const QByteArray &type, const QByteArray &description,
                            diagnostics::Level level)
{
    const QByteArray message =
        description.isEmpty() ? type : type + QByteArrayLiteral(": ") + description;
    return sentry_value_new_message_event_n(sentryLevel(level), nullptr, 0, message.constData(),
                                            size_t(message.size()));
}

sentry_value_t keepCrash(const sentry_ucontext_t *, sentry_value_t event, void *)
{
    return event;
}

void setPaths(sentry_options_t *options, const diagnostics::backend::Configuration &configuration)
{
#ifdef Q_OS_WIN
    sentry_options_set_database_pathw(
        options, reinterpret_cast<const wchar_t *>(configuration.databasePath.utf16()));
    sentry_options_set_handler_pathw(
        options, reinterpret_cast<const wchar_t *>(configuration.handlerPath.utf16()));
#else
    sentry_options_set_database_path(options,
                                     QFile::encodeName(configuration.databasePath).constData());
    sentry_options_set_handler_path(options,
                                    QFile::encodeName(configuration.handlerPath).constData());
#endif
}

}

namespace diagnostics::backend {

bool compiled()
{
    return true;
}

bool start(const Configuration &configuration)
{
    if (started.load())
        return true;

    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options, configuration.dsn.toUtf8().constData());
    sentry_options_set_release(options, configuration.release.toUtf8().constData());
    sentry_options_set_environment(options, configuration.environment.toUtf8().constData());
    setPaths(options, configuration);
    sentry_options_set_max_breadcrumbs(options, kMaximumBreadcrumbs);
    sentry_options_set_auto_session_tracking(options, 0);
    sentry_options_set_require_user_consent(options, 1);
    sentry_options_set_shutdown_timeout(options, kShutdownTimeoutMs);
    sentry_options_set_before_send(options, &withoutLocalPaths, nullptr);
    sentry_options_set_on_crash(options, &keepCrash, nullptr);
    sentry_options_set_debug(options, configuration.verbose ? 1 : 0);
    if (configuration.verbose)
        sentry_options_set_logger_level(options, SENTRY_LEVEL_TRACE);

    if (sentry_init(options) != 0)
        return false;

    sentry_user_consent_give();
    started.store(true);
    return true;
}

void stop(Ending ending)
{
    if (!started.exchange(false))
        return;
    if (ending == Ending::Revoked)
        sentry_user_consent_revoke();
    sentry_close();
}

bool running()
{
    return started.load();
}

void addBreadcrumb(const char *category, const QByteArray &message, std::span<const Field> fields,
                   Level level)
{
    if (!started.load())
        return;

    const QByteArray text = message.isEmpty() ? summaryOf(fields) : message;
    const sentry_value_t breadcrumb =
        sentry_value_new_breadcrumb(breadcrumbType(QByteArrayView(category), level),
                                    text.isEmpty() ? nullptr : text.constData());
    sentry_value_set_by_key(breadcrumb, "category", sentry_value_new_string(category));
    sentry_value_set_by_key(breadcrumb, "level", sentry_value_new_string(levelName(level)));
    if (!fields.empty())
        sentry_value_set_by_key(breadcrumb, "data", sentryObject(fields));
    sentry_add_breadcrumb(breadcrumb);
}

void capture(const Event &event)
{
    if (!started.load())
        return;

    const QByteArray description =
        event.description.isEmpty() ? summaryOf(event.fields) : event.description;
    const QByteArray logger =
        event.logger.isEmpty() ? QByteArrayLiteral("diagnostics") : event.logger;

    const sentry_value_t value = event.stacktrace
        ? exceptionEvent(event.type, description)
        : messageEvent(event.type, description, event.level);
    sentry_event_set_level(value, sentryLevel(event.level));
    sentry_value_set_by_key(value, "logger", sentry_value_new_string(logger.constData()));

    if (!event.fields.isEmpty())
        sentry_value_set_by_key(value, "extra", sentryObject(event.fields));

    if (!event.fingerprint.isEmpty()) {
        const sentry_value_t fingerprint = sentry_value_new_list();
        for (const QByteArray &part : event.fingerprint)
            sentry_value_append(fingerprint,
                                sentry_value_new_string_n(part.constData(), size_t(part.size())));
        sentry_value_set_by_key(value, "fingerprint", fingerprint);
    }

    sentry_capture_event(value);
}

void setTag(const char *key, const QByteArray &value)
{
    if (started.load())
        sentry_set_tag_n(key, std::strlen(key), value.constData(), size_t(value.size()));
}

void setContext(const char *name, std::span<const Field> fields)
{
    if (started.load())
        sentry_set_context(name, sentryObject(fields));
}

void crash()
{
    if (started.load())
        sentry_crash();
}

}
