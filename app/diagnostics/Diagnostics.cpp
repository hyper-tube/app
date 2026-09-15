#include "Diagnostics.h"

#include "Backend.h"
#include "core/Logging.h"

#include <QByteArray>
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>

#include <cstdlib>
#include <exception>
#include <span>
#include <typeinfo>
#include <utility>

#if __has_include(<cxxabi.h>)
#include <cxxabi.h>
#endif

namespace {

constexpr qint64 kRepeatWindowMs = 1000;

struct UncaughtException
{
    QByteArray type;
    QByteArray what;
};

std::terminate_handler previousTermination = nullptr;

std::span<const diagnostics::Field> spanOf(diagnostics::Fields fields)
{
    return {fields.begin(), fields.size()};
}

class Repeats
{
public:
    static Repeats &instance()
    {
        static Repeats repeats;
        return repeats;
    }

    bool repeated(const char *category, diagnostics::Fields fields)
    {
        QByteArray key(category);
        for (const diagnostics::Field &field : fields)
            key += ' ' + QByteArray(field.key) + '=' + field.value.text();

        QMutexLocker const locker(&m_mutex);
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        const bool repeated = key == m_last && now - m_lastAt < kRepeatWindowMs;
        m_last = std::move(key);
        m_lastAt = now;
        return repeated;
    }

private:
    QMutex m_mutex;
    QByteArray m_last;
    qint64 m_lastAt = 0;
};

QByteArray readableName(const std::type_info &type)
{
#if __has_include(<cxxabi.h>)
    int status = 0;
    char *demangled = abi::__cxa_demangle(type.name(), nullptr, nullptr, &status);
    if (demangled) {
        QByteArray name(demangled);
        std::free(demangled);
        return name;
    }
#endif
    return QByteArray(type.name());
}

UncaughtException uncaughtException()
{
    const std::exception_ptr current = std::current_exception();
    if (!current)
        return {QByteArrayLiteral("none"), {}};
    try {
        std::rethrow_exception(current);
    } catch (const std::exception &error) {
        return {readableName(typeid(error)), QByteArray(error.what())};
    } catch (...) {
#if __has_include(<cxxabi.h>)
        if (const std::type_info *type = abi::__cxa_current_exception_type())
            return {readableName(*type), {}};
#endif
    }
    return {QByteArrayLiteral("unknown"), {}};
}

void reportTermination()
{
    const UncaughtException exception = uncaughtException();
    qCWarning(logDiagnostics).noquote()
        << "terminating on an uncaught exception" << exception.type << exception.what;
    diagnostics::breadcrumb(
        "cpp.uncaught_exception",
        {{"type", diagnostics::Value::symbol(QLatin1StringView(exception.type))}},
        diagnostics::Level::Fatal);

    if (previousTermination)
        previousTermination();
    std::abort();
}

diagnostics::backend::Event eventOf(const char *name, diagnostics::Fields fields,
                                    diagnostics::Level level, bool stacktrace)
{
    diagnostics::backend::Event event;
    event.type = QByteArray(name);
    event.level = level;
    event.fields = QList<diagnostics::Field>(fields.begin(), fields.end());
    event.fingerprint = {QByteArray(name)};
    event.stacktrace = stacktrace;
    return event;
}

}

namespace diagnostics {

bool active()
{
    return backend::running();
}

void breadcrumb(const char *category, Fields fields, Level level)
{
    if (backend::running() && !Repeats::instance().repeated(category, fields))
        backend::addBreadcrumb(category, {}, spanOf(fields), level);
}

void captureMessage(const char *name, Fields fields, Level level)
{
    if (backend::running())
        backend::capture(eventOf(name, fields, level, false));
}

void captureError(const char *name, Fields fields)
{
    if (backend::running())
        backend::capture(eventOf(name, fields, Level::Error, true));
}

void setTag(const Field &tag)
{
    if (backend::running())
        backend::setTag(tag.key, tag.value.text());
}

void setContext(const char *name, Fields fields)
{
    if (backend::running())
        backend::setContext(name, spanOf(fields));
}

void guardTermination()
{
    previousTermination = std::set_terminate(&reportTermination);
}

void shutDown()
{
    backend::stop(backend::Ending::Shutdown);
}

}
