#include "LogBridge.h"

#include "Backend.h"
#include "core/Logging.h"

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QMutex>
#include <QMutexLocker>
#include <QSet>

#include <algorithm>
#include <array>
#include <utility>

namespace {

constexpr qint64 kRepeatWindowMs = 1000;
constexpr qsizetype kCategoryLimit = 64;
constexpr qsizetype kFileLimit = 64;
constexpr qsizetype kFunctionLimit = 120;

struct ScriptFailure
{
    QLatin1StringView marker;
    const char *kind;
};

constexpr std::array kScriptFailures {
    ScriptFailure {QLatin1StringView("TypeError"), "type_error"},
    ScriptFailure {QLatin1StringView("ReferenceError"), "reference_error"},
    ScriptFailure {QLatin1StringView("RangeError"), "range_error"},
    ScriptFailure {QLatin1StringView("SyntaxError"), "syntax_error"},
    ScriptFailure {QLatin1StringView("Binding loop detected"), "binding_loop"},
    ScriptFailure {QLatin1StringView("Unable to assign"), "assignment"},
    ScriptFailure {QLatin1StringView("Cannot assign"), "assignment"},
    ScriptFailure {QLatin1StringView("anchor"), "anchors"},
    ScriptFailure {QLatin1StringView("is not a type"), "import"},
    ScriptFailure {QLatin1StringView("is not installed"), "import"},
    ScriptFailure {QLatin1StringView("Component is not ready"), "component"},
    ScriptFailure {QLatin1StringView("Error:"), "error"},
};

constexpr std::array kSourceSuffixes {
    QByteArrayView(".cpp"), QByteArrayView(".h"),   QByteArrayView(".mm"), QByteArrayView(".c"),
    QByteArrayView(".cc"),  QByteArrayView(".qml"), QByteArrayView(".js"), QByteArrayView(".mjs"),
};

constexpr std::array kScriptSuffixes {
    QByteArrayView(".qml"),
    QByteArrayView(".js"),
    QByteArrayView(".mjs"),
};

thread_local bool forwarding = false;

class Reentry
{
public:
    Reentry() { forwarding = true; }
    ~Reentry() { forwarding = false; }
    Reentry(const Reentry &) = delete;
    Reentry &operator=(const Reentry &) = delete;
};

class Throttle
{
public:
    static Throttle &instance()
    {
        static Throttle throttle;
        return throttle;
    }

    bool admitsWarning(const QByteArray &site)
    {
        QMutexLocker const locker(&m_mutex);
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        const bool repeated = site == m_lastSite && now - m_lastAt < kRepeatWindowMs;
        m_lastSite = site;
        m_lastAt = now;
        return !repeated;
    }

    bool firstCritical(const QByteArray &site)
    {
        QMutexLocker const locker(&m_mutex);
        if (m_captured.contains(site))
            return false;
        m_captured.insert(site);
        return true;
    }

private:
    QMutex m_mutex;
    QByteArray m_lastSite;
    qint64 m_lastAt = 0;
    QSet<QByteArray> m_captured;
};

bool lettersOrDigits(char character)
{
    return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z')
        || (character >= '0' && character <= '9');
}

bool consistsOf(QByteArrayView text, qsizetype limit, bool (*allowed)(char))
{
    return !text.isEmpty() && text.size() <= limit && std::ranges::all_of(text, allowed);
}

QByteArray checkedCategory(QLatin1StringView category)
{
    const QByteArrayView text(category.data(), category.size());
    const bool valid = consistsOf(text, kCategoryLimit, [](char character) {
        return (character >= 'a' && character <= 'z') || (character >= '0' && character <= '9')
            || character == '.' || character == '_' || character == '-';
    });
    return valid ? text.toByteArray() : QByteArrayLiteral("other");
}

QByteArray checkedFile(QLatin1StringView file)
{
    const QByteArrayView text(file.data(), file.size());
    const bool valid = consistsOf(text, kFileLimit, [](char character) {
        return lettersOrDigits(character) || character == '.' || character == '_'
            || character == '-' || character == '+';
    });
    const bool source = std::ranges::any_of(
        kSourceSuffixes, [&text](QByteArrayView suffix) { return text.endsWith(suffix); });
    return valid && source ? text.toByteArray() : QByteArray();
}

QByteArray checkedFunction(const QString &function)
{
    const QByteArray text = function.toLatin1();
    const bool valid = consistsOf(text, kFunctionLimit, [](char character) {
        return lettersOrDigits(character) || character == '_' || character == ':'
            || character == '~' || character == '<' || character == '>' || character == ' '
            || character == '.' || character == '-';
    });
    return valid ? text : QByteArray();
}

bool script(QByteArrayView file)
{
    return std::ranges::any_of(kScriptSuffixes,
                               [&file](QByteArrayView suffix) { return file.endsWith(suffix); });
}

const char *scriptFailure(QStringView message)
{
    for (const ScriptFailure &failure : kScriptFailures) {
        if (message.contains(failure.marker))
            return failure.kind;
    }
    return "other";
}

diagnostics::Level levelOf(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg: return diagnostics::Level::Debug;
    case QtInfoMsg: return diagnostics::Level::Info;
    case QtWarningMsg: return diagnostics::Level::Warning;
    case QtCriticalMsg: return diagnostics::Level::Error;
    case QtFatalMsg: break;
    }
    return diagnostics::Level::Fatal;
}

QByteArray locationOf(const QByteArray &file, int line, const QByteArray &function)
{
    if (file.isEmpty())
        return QByteArrayLiteral("unknown location");
    QByteArray location = file + ':' + QByteArray::number(line);
    if (!function.isEmpty())
        location += QByteArrayLiteral(" (") + function + ')';
    return location;
}

void forward(const core::logging::Record &record)
{
    if (record.type == QtDebugMsg || record.type == QtInfoMsg || forwarding
        || !diagnostics::backend::running())
        return;
    const Reentry reentry;

    const QByteArray category = checkedCategory(record.category);
    const QByteArray file = checkedFile(record.file);
    const QByteArray function = file.isEmpty() ? QByteArray() : checkedFunction(record.function);
    const QByteArray location = locationOf(file, record.line, function);
    const QByteArray site = category + ' ' + file + ':' + QByteArray::number(record.line);

    if (record.type == QtWarningMsg && !Throttle::instance().admitsWarning(site))
        return;

    QList<diagnostics::Field> fields;
    if (!file.isEmpty()) {
        fields.append(
            diagnostics::Field {"file", diagnostics::Value::symbol(QLatin1StringView(file))});
        fields.append(diagnostics::Field {"line", record.line});
    }
    if (script(file))
        fields.append(diagnostics::Field {"qml_error", scriptFailure(record.message)});
    fields.append(diagnostics::Field {
        "thread", record.thread == QLatin1StringView("main") ? "main" : "worker"});

    const diagnostics::Level level = levelOf(record.type);
    const QByteArray breadcrumbCategory = QByteArrayLiteral("log.") + category;
    diagnostics::backend::addBreadcrumb(breadcrumbCategory.constData(), location, fields, level);

    if (record.type != QtCriticalMsg || !Throttle::instance().firstCritical(site))
        return;

    diagnostics::backend::Event event;
    event.type = category;
    event.description = location;
    event.logger = category;
    event.level = level;
    event.fields = std::move(fields);
    event.fingerprint = {QByteArrayLiteral("log"), category, file, QByteArray::number(record.line)};
    diagnostics::backend::capture(event);
}

}

namespace diagnostics::logBridge {

void install()
{
    core::logging::observe(&forward);
}

}
