#include "DiscordIpc.h"

#include "core/Logging.h"

#include <QJsonDocument>
#include <QLocalSocket>
#include <QtEndian>

#include <utility>

namespace {

constexpr int kHeaderSize = 8;
constexpr quint32 kMaximumFrame = 1U << 20U;
constexpr int kSocketRange = 10;

constexpr quint32 kHandshake = 0;
constexpr quint32 kFrame = 1;
constexpr quint32 kCloseFrame = 2;
constexpr quint32 kPing = 3;
constexpr quint32 kPong = 4;

const QStringList kEnvironmentBases = {
    QStringLiteral("XDG_RUNTIME_DIR"),
    QStringLiteral("TMPDIR"),
    QStringLiteral("TMP"),
    QStringLiteral("TEMP"),
};

const QStringList kFlavourDirectories = {
    QString(),
    QStringLiteral("app/com.discordapp.Discord"),
    QStringLiteral("app/com.discordapp.DiscordCanary"),
    QStringLiteral("app/com.discordapp.DiscordPTB"),
    QStringLiteral("snap.discord"),
    QStringLiteral("snap.discord-canary"),
};

}

namespace plugin::discord {

DiscordIpc::DiscordIpc(QObject *parent)
    : QObject(parent)
    , m_socket(new QLocalSocket(this))
{
    connect(m_socket, &QLocalSocket::connected, this, &DiscordIpc::handshake);
    connect(m_socket, &QLocalSocket::readyRead, this, &DiscordIpc::readFrames);
    connect(m_socket, &QLocalSocket::errorOccurred, this, [this] {
        if (m_ready)
            giveUp();
        else
            tryNext();
    });
    connect(m_socket, &QLocalSocket::disconnected, this, &DiscordIpc::giveUp);
}

QStringList DiscordIpc::candidates()
{
    QStringList names;
#ifdef Q_OS_WIN
    for (int index = 0; index < kSocketRange; ++index)
        names.append(QStringLiteral("discord-ipc-%1").arg(index));
#else
    QStringList bases;
    for (const QString &variable : kEnvironmentBases) {
        const QString value = qEnvironmentVariable(variable.toLatin1().constData());
        if (!value.isEmpty() && !bases.contains(value))
            bases.append(value);
    }
    if (!bases.contains(QStringLiteral("/tmp")))
        bases.append(QStringLiteral("/tmp"));

    for (const QString &base : std::as_const(bases)) {
        for (const QString &flavour : kFlavourDirectories) {
            const QString directory = flavour.isEmpty() ? base : base + QLatin1Char('/') + flavour;
            for (int index = 0; index < kSocketRange; ++index)
                names.append(directory + QStringLiteral("/discord-ipc-%1").arg(index));
        }
    }
#endif
    return names;
}

void DiscordIpc::open(const QString &applicationId)
{
    if (m_ready || !m_candidates.isEmpty())
        return;
    m_applicationId = applicationId;
    m_candidates = candidates();
    tryNext();
}

void DiscordIpc::close()
{
    const bool announce = m_ready;
    m_candidates.clear();
    m_buffer.clear();
    m_ready = false;
    m_socket->abort();
    if (announce)
        Q_EMIT closed();
}

void DiscordIpc::tryNext()
{
    m_socket->abort();
    m_buffer.clear();
    if (m_candidates.isEmpty()) {
        Q_EMIT closed();
        return;
    }
    const QString name = m_candidates.takeFirst();
    m_socket->connectToServer(name);
}

void DiscordIpc::giveUp()
{
    const bool announce = m_ready || !m_candidates.isEmpty();
    m_candidates.clear();
    m_buffer.clear();
    m_ready = false;
    m_socket->abort();
    if (announce)
        Q_EMIT closed();
}

void DiscordIpc::handshake()
{
    m_candidates.clear();
    writeFrame(kHandshake,
               {{QStringLiteral("v"), 1}, {QStringLiteral("client_id"), m_applicationId}});
}

void DiscordIpc::send(const QJsonObject &payload)
{
    if (!m_ready)
        return;
    writeFrame(kFrame, payload);
}

void DiscordIpc::writeFrame(quint32 opcode, const QJsonObject &payload)
{
    const QByteArray body = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    QByteArray frame(kHeaderSize, Qt::Uninitialized);
    qToLittleEndian(opcode, frame.data());
    qToLittleEndian(quint32(body.size()), frame.data() + 4);
    frame.append(body);
    m_socket->write(frame);
}

void DiscordIpc::readFrames()
{
    m_buffer.append(m_socket->readAll());
    while (m_buffer.size() >= kHeaderSize) {
        const quint32 opcode = qFromLittleEndian<quint32>(m_buffer.constData());
        const quint32 length = qFromLittleEndian<quint32>(m_buffer.constData() + 4);
        if (length > kMaximumFrame) {
            giveUp();
            return;
        }
        if (std::cmp_less(m_buffer.size() - kHeaderSize, length))
            return;
        const QByteArray payload = m_buffer.mid(kHeaderSize, int(length));
        m_buffer.remove(0, kHeaderSize + int(length));
        dispatch(opcode, payload);
    }
}

void DiscordIpc::dispatch(quint32 opcode, const QByteArray &payload)
{
    const QJsonObject object = QJsonDocument::fromJson(payload).object();
    if (opcode == kPing) {
        writeFrame(kPong, object);
        return;
    }
    if (opcode == kCloseFrame) {
        const QString message = object.value(QStringLiteral("message")).toString();
        qCInfo(logPlugins) << "discord closed the connection" << message;
        if (m_ready) {
            giveUp();
            return;
        }
        m_candidates.clear();
        m_socket->abort();
        Q_EMIT rejected(message);
        return;
    }
    if (opcode != kFrame)
        return;
    if (object.value(QStringLiteral("evt")).toString() == QLatin1String("READY")) {
        m_ready = true;
        Q_EMIT ready();
        return;
    }
    if (object.value(QStringLiteral("evt")).toString() == QLatin1String("ERROR")) {
        const QJsonObject data = object.value(QStringLiteral("data")).toObject();
        qCWarning(logPlugins) << "discord rejected a command"
                              << data.value(QStringLiteral("message")).toString();
    }
}

}
