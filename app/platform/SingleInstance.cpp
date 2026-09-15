#include "SingleInstance.h"

#include "core/AppInfo.h"
#include "core/Logging.h"
#include "core/Paths.h"

#include <QCryptographicHash>
#include <QCoreApplication>
#include <QTimer>
#include <QLocalSocket>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {

constexpr int kHandshakeTimeoutMs = 300;
constexpr int kScopeLength = 8;

QString socketName()
{
    const QByteArray scope =
        QCryptographicHash::hash(core::paths::configDir().toUtf8(), QCryptographicHash::Sha1);
    return core::AppInfo::identifier() + QLatin1Char('-')
        + QString::fromLatin1(scope.toHex().left(kScopeLength));
}

void lendForeground(const QLocalSocket &running)
{
#ifdef Q_OS_WIN
    ULONG owner = 0;
    if (::GetNamedPipeServerProcessId(reinterpret_cast<HANDLE>(running.socketDescriptor()), &owner))
        ::AllowSetForegroundWindow(owner);
#else
    Q_UNUSED(running)
#endif
}

}

namespace platform {

SingleInstance::SingleInstance(QObject *parent)
    : QObject(parent)
{
    connect(&m_server, &QLocalServer::newConnection, this, [this] {
        while (QLocalSocket *client = m_server.nextPendingConnection()) {
            connect(client, &QLocalSocket::readyRead, this, [this, client] {
                const QByteArray command = client->readAll().trimmed();
                if (command == "quit")
                    QCoreApplication::quit();
                else
                    Q_EMIT raiseRequested();
                client->disconnectFromServer();
            });
            connect(client, &QLocalSocket::disconnected, client, &QObject::deleteLater);
            QTimer::singleShot(2000, client, [client] { client->disconnectFromServer(); });
        }
    });
}

bool SingleInstance::claim()
{
    const QString name = socketName();

    QLocalSocket running;
    running.connectToServer(name);
    if (running.waitForConnected(kHandshakeTimeoutMs)) {
        qCInfo(logPlatform) << "another instance owns" << name << "raising it";
        lendForeground(running);
        running.write("raise\n");
        running.waitForBytesWritten(kHandshakeTimeoutMs);
        running.disconnectFromServer();
        return false;
    }

    QLocalServer::removeServer(name);
    m_server.setSocketOptions(QLocalServer::UserAccessOption);
    if (!m_server.listen(name))
        qCWarning(logPlatform) << "cannot own" << name << m_server.errorString();
    return true;
}

}
