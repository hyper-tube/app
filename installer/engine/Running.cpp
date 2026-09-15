#include "Running.h"

#include "core/AppInfo.h"
#include "core/Paths.h"

#include <QDeadlineTimer>
#include <QCryptographicHash>
#include <QLocalSocket>
#include <QDir>
#include <QFileInfo>
#include <QThread>

#include <windows.h>

#include <tlhelp32.h>

namespace {

QString imageOf(DWORD id)
{
    const HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, id);
    if (!process)
        return {};

    wchar_t buffer[MAX_PATH * 4] = {};
    DWORD size = DWORD(std::size(buffer));
    QString image;
    if (QueryFullProcessImageNameW(process, 0, buffer, &size))
        image = QDir::fromNativeSeparators(QString::fromWCharArray(buffer, int(size)));
    CloseHandle(process);
    return image;
}

QList<DWORD> identifiersInside(const QString &directory)
{
    QList<DWORD> found;
    const QString root = QDir::cleanPath(directory) + QLatin1Char('/');
    const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return found;

    PROCESSENTRY32W entry {};
    entry.dwSize = sizeof(entry);
    const DWORD self = GetCurrentProcessId();
    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (entry.th32ProcessID == self)
                continue;
            const QString image = imageOf(entry.th32ProcessID);
            if (!image.isEmpty() && image.startsWith(root, Qt::CaseInsensitive))
                found.append(entry.th32ProcessID);
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return found;
}

BOOL CALLBACK closeWindow(HWND window, LPARAM target)
{
    DWORD owner = 0;
    GetWindowThreadProcessId(window, &owner);
    if (owner == DWORD(target))
        PostMessageW(window, WM_CLOSE, 0, 0);
    return TRUE;
}

}

namespace setup::running {

QStringList inside(const QString &directory)
{
    QStringList names;
    for (DWORD id : identifiersInside(directory)) {
        const QString image = imageOf(id);
        const QString name = QFileInfo(image).fileName();
        if (!name.isEmpty() && !names.contains(name))
            names.append(name);
    }
    return names;
}

bool askToClose(const QString &directory, int millisecondsToWait)
{
    const QList<DWORD> identifiers = identifiersInside(directory);
    if (identifiers.isEmpty())
        return true;

    const QByteArray scope =
        QCryptographicHash::hash(core::paths::configDir().toUtf8(), QCryptographicHash::Sha1);
    QLocalSocket application;
    application.connectToServer(core::AppInfo::identifier() + QLatin1Char('-')
                                + QString::fromLatin1(scope.toHex().left(8)));
    if (application.waitForConnected(300)) {
        application.write("quit\n");
        application.waitForBytesWritten(300);
        application.disconnectFromServer();
    }
    for (DWORD id : identifiers)
        EnumWindows(closeWindow, LPARAM(id));

    QDeadlineTimer deadline(millisecondsToWait);
    while (!deadline.hasExpired()) {
        if (identifiersInside(directory).isEmpty())
            return true;
        QThread::msleep(200);
    }
    return identifiersInside(directory).isEmpty();
}

}
