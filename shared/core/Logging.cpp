#include "Logging.h"

#include "BuildInfo.h"
#include "Paths.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QSysInfo>
#include <QTextStream>
#include <QThread>

#include <array>
#include <atomic>
#include <cstring>

Q_LOGGING_CATEGORY(logTheme, "htmusic.theme")
Q_LOGGING_CATEGORY(logPlayback, "htmusic.playback")
Q_LOGGING_CATEGORY(logArtwork, "htmusic.artwork")
Q_LOGGING_CATEGORY(logNet, "htmusic.net")
Q_LOGGING_CATEGORY(logInnerTube, "htmusic.innertube")
Q_LOGGING_CATEGORY(logStream, "htmusic.stream")
Q_LOGGING_CATEGORY(logTransition, "htmusic.transition")
Q_LOGGING_CATEGORY(logPlatform, "htmusic.platform")
Q_LOGGING_CATEGORY(logPlugins, "htmusic.plugins")
Q_LOGGING_CATEGORY(logDiagnostics, "htmusic.diagnostics")

namespace {

constexpr qint64 kRotateAtBytes = 2 * 1024 * 1024;
constexpr int kKeptFiles = 3;
constexpr qsizetype kFunctionNameLimit = 120;

constexpr QLatin1StringView kDefaultCategory("default");
constexpr QLatin1StringView kLambda("<lambda>");
constexpr QLatin1StringView kScope("::");
constexpr QLatin1StringView kOperatorCall("::operator");
constexpr QLatin1StringView kTemplateArguments(" [with ");
constexpr std::array kAnonymousNamespaces {
    QLatin1StringView("{anonymous}::"),
    QLatin1StringView("`anonymous namespace'::"),
};
constexpr std::array kTrailingQualifiers {
    QLatin1StringView(" const"),    QLatin1StringView(" volatile"), QLatin1StringView(" noexcept"),
    QLatin1StringView(" override"), QLatin1StringView(" &&"),       QLatin1StringView(" &"),
};

QLatin1StringView levelOf(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg: return QLatin1StringView("DEBUG");
    case QtInfoMsg: return QLatin1StringView("INFO");
    case QtWarningMsg: return QLatin1StringView("WARN");
    case QtCriticalMsg: return QLatin1StringView("CRIT");
    case QtFatalMsg: break;
    }
    return QLatin1StringView("FATAL");
}

QString replacementForGroup(QStringView group)
{
    if (group.startsWith(QLatin1StringView("anonymous namespace")))
        return {};
    if (group.startsWith(QLatin1StringView("anonymous"))
        || group.startsWith(QLatin1StringView("lambda")))
        return kLambda;
    return {};
}

QString withoutParameterLists(QStringView signature)
{
    QString result;
    result.reserve(signature.size());
    int depth = 0;
    qsizetype groupStart = 0;
    for (qsizetype index = 0; index < signature.size(); ++index) {
        const QChar character = signature.at(index);
        if (character == QLatin1Char('(')) {
            if (depth++ == 0)
                groupStart = index + 1;
        } else if (character == QLatin1Char(')') && depth > 0) {
            if (--depth == 0)
                result += replacementForGroup(signature.sliced(groupStart, index - groupStart));
        } else if (depth == 0) {
            result += character;
        }
    }
    return result;
}

QString withoutTrailingQualifiers(QString name)
{
    bool stripped = true;
    while (stripped) {
        name = name.trimmed();
        stripped = false;
        for (const QLatin1StringView qualifier : kTrailingQualifiers) {
            if (name.endsWith(qualifier)) {
                name.chop(qualifier.size());
                stripped = true;
            }
        }
    }
    return name;
}

QString withoutReturnType(const QString &name)
{
    int angleDepth = 0;
    qsizetype lastSpace = -1;
    for (qsizetype index = 0; index < name.size(); ++index) {
        const QChar character = name.at(index);
        if (character == QLatin1Char('<'))
            ++angleDepth;
        else if (character == QLatin1Char('>'))
            angleDepth = qMax(0, angleDepth - 1);
        else if (character == QLatin1Char(' ') && angleDepth == 0)
            lastSpace = index;
    }
    return lastSpace < 0 ? name : name.mid(lastSpace + 1);
}

QString threadName()
{
    if (QThread::isMainThread())
        return QStringLiteral("main");
    const QString name = QThread::currentThread()->objectName();
    if (!name.isEmpty())
        return name;
    return QStringLiteral("0x") + QString::number(quintptr(QThread::currentThreadId()), 16);
}

QString formatted(const core::logging::Record &record)
{
    QString line = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    line += QStringLiteral(" [") + levelOf(record.type) + QStringLiteral("] [") + record.category
        + QStringLiteral("] [") + record.thread + QLatin1Char(']');
    if (!record.file.isEmpty())
        line += QLatin1Char(' ') + record.file + QLatin1Char(':') + QString::number(record.line);
    if (!record.function.isEmpty())
        line += QStringLiteral(" (") + record.function + QLatin1Char(')');
    line += QLatin1Char(' ');
    line += record.message;
    return line;
}

class FileSink
{
public:
    static FileSink &instance()
    {
        static FileSink sink;
        return sink;
    }

    bool open(const QString &path)
    {
        QMutexLocker const locker(&m_mutex);
        m_file.setFileName(path);
        return m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    }

    void write(const QString &line)
    {
        QMutexLocker const locker(&m_mutex);
        if (!m_file.isOpen())
            return;

        QTextStream stream(&m_file);
        stream << line << '\n';
        stream.flush();

        if (m_file.size() > kRotateAtBytes)
            rotate();
    }

private:
    void rotate()
    {
        const QString base = m_file.fileName();
        m_file.close();

        const auto kept = [&base](int index) { return base + QStringLiteral(".%1").arg(index); };

        QFile::remove(kept(kKeptFiles));
        for (int index = kKeptFiles; index > 1; --index)
            QFile::rename(kept(index - 1), kept(index));
        QFile::rename(base, kept(1));

        if (!m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
            m_file.close();
    }

    QMutex m_mutex;
    QFile m_file;
};

QtMessageHandler previousHandler = nullptr;
std::atomic<core::logging::Observer> currentObserver = nullptr;
QString chosenPath;

void handle(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    if (previousHandler)
        previousHandler(type, context, message);

    core::logging::Record record;
    record.type = type;
    record.category = context.category ? QLatin1StringView(context.category) : kDefaultCategory;
    record.file = core::logging::fileName(context.file);
    record.line = context.line;
    record.function = core::logging::functionName(context.function);
    record.thread = threadName();
    record.message = message;

    FileSink::instance().write(formatted(record));

    if (const core::logging::Observer observer = currentObserver.load())
        observer(record);
}

}

namespace core::logging {

void install(const QString &path)
{
    chosenPath = path;
    QDir().mkpath(QFileInfo(file()).absolutePath());
    if (!FileSink::instance().open(file()))
        return;

    const QString commit = QString::fromLatin1(build::kCommit);
    FileSink::instance().write(
        QStringLiteral("--- %1 %2%3, Qt %4, %5 %6, started %7")
            .arg(QCoreApplication::applicationName(), QCoreApplication::applicationVersion(),
                 commit.isEmpty() ? QString() : QStringLiteral(" (%1)").arg(commit),
                 QString::fromLatin1(qVersion()), QSysInfo::prettyProductName(),
                 QSysInfo::currentCpuArchitecture(),
                 QDateTime::currentDateTime().toString(Qt::ISODate)));

    previousHandler = qInstallMessageHandler(handle);
}

void observe(Observer observer)
{
    currentObserver.store(observer);
}

QLatin1StringView fileName(const char *path)
{
    if (!path)
        return {};
    const QLatin1StringView full(path, qsizetype(std::strlen(path)));
    const qsizetype separator =
        qMax(full.lastIndexOf(QLatin1Char('/')), full.lastIndexOf(QLatin1Char('\\')));
    return full.sliced(separator + 1);
}

QString functionName(const char *signature)
{
    if (!signature || !*signature)
        return {};

    QString name = QString::fromLatin1(signature);
    const qsizetype templateArguments = name.indexOf(kTemplateArguments);
    if (templateArguments >= 0)
        name.truncate(templateArguments);
    for (const QLatin1StringView anonymous : kAnonymousNamespaces)
        name.remove(anonymous);

    name = withoutReturnType(withoutTrailingQualifiers(withoutParameterLists(name)));
    if (name.endsWith(kOperatorCall))
        name.chop(kOperatorCall.size());
    while (name.contains(QStringLiteral("::::")))
        name.replace(QStringLiteral("::::"), kScope);
    if (name.startsWith(kScope))
        name.remove(0, kScope.size());
    if (name.size() > kFunctionNameLimit)
        name = name.right(kFunctionNameLimit);
    return name;
}

QString folder()
{
    return core::paths::logDir();
}

QString file()
{
    if (!chosenPath.isEmpty())
        return chosenPath;
    return folder() + QLatin1Char('/') + QCoreApplication::applicationName()
        + QStringLiteral(".log");
}

}
