#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>

class QLocalSocket;

namespace plugin::discord {

class DiscordIpc : public QObject
{
    Q_OBJECT

public:
    explicit DiscordIpc(QObject *parent);

    bool connected() const { return m_ready; }
    bool connecting() const { return !m_ready && !m_candidates.isEmpty(); }

    void open(const QString &applicationId);
    void close();
    void send(const QJsonObject &payload);

Q_SIGNALS:
    void ready();
    void closed();
    void rejected(const QString &message);

private:
    void tryNext();
    void handshake();
    void readFrames();
    void writeFrame(quint32 opcode, const QJsonObject &payload);
    void dispatch(quint32 opcode, const QByteArray &payload);
    void giveUp();

    static QStringList candidates();

    QLocalSocket *m_socket;
    QByteArray m_buffer;
    QStringList m_candidates;
    QString m_applicationId;
    bool m_ready = false;
};

}
