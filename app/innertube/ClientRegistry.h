#pragma once

#include <QHash>
#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace innertube {

struct Client
{
    QString key;
    QString name;
    QString version;
    QString id;
    QString userAgent;
    QString osName;
    QString osVersion;
    QString deviceMake;
    QString deviceModel;
    QString androidSdkVersion;
    bool loginSupported = false;
};

class ClientRegistry
{
public:
    ClientRegistry();

    const Client *client(const QString &key) const;
    const Client *metadataClient() const;
    const Client *lyricsClient() const;
    const QStringList &streamChain() const { return m_streamChain; }
    const QStringList &uploadChain() const { return m_uploadChain; }

private:
    bool loadFrom(const QString &path);
    QStringList readChain(const QJsonObject &root, const QString &key);

    QHash<QString, Client> m_clients;
    QStringList m_streamChain;
    QStringList m_uploadChain;
    QString m_metadataClient;
    QString m_lyricsClient;
};

}
