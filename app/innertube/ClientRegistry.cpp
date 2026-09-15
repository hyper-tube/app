#include "ClientRegistry.h"

#include "core/Logging.h"
#include "core/Paths.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

const QString kBundledPath = QStringLiteral(":/innertube/clients.json");

QString overridePath()
{
    return core::paths::configDir() + QStringLiteral("/clients.json");
}

innertube::Client readClient(const QString &key, const QJsonObject &json)
{
    innertube::Client client;
    client.key = key;
    client.name = json.value(QStringLiteral("clientName")).toString();
    client.version = json.value(QStringLiteral("clientVersion")).toString();
    client.id = json.value(QStringLiteral("clientId")).toString();
    client.userAgent = json.value(QStringLiteral("userAgent")).toString();
    client.osName = json.value(QStringLiteral("osName")).toString();
    client.osVersion = json.value(QStringLiteral("osVersion")).toString();
    client.deviceMake = json.value(QStringLiteral("deviceMake")).toString();
    client.deviceModel = json.value(QStringLiteral("deviceModel")).toString();
    client.androidSdkVersion = json.value(QStringLiteral("androidSdkVersion")).toString();
    client.loginSupported = json.value(QStringLiteral("loginSupported")).toBool();
    return client;
}

}

namespace innertube {

ClientRegistry::ClientRegistry()
{
    if (!loadFrom(overridePath()) && !loadFrom(kBundledPath))
        qCWarning(logInnerTube) << "no client registry available";
}

const Client *ClientRegistry::client(const QString &key) const
{
    const auto it = m_clients.constFind(key);
    if (it == m_clients.constEnd()) {
        qCWarning(logInnerTube) << "unknown client" << key;
        return nullptr;
    }
    return &it.value();
}

const Client *ClientRegistry::metadataClient() const
{
    return m_metadataClient.isEmpty() ? nullptr : client(m_metadataClient);
}

const Client *ClientRegistry::lyricsClient() const
{
    return m_lyricsClient.isEmpty() ? nullptr : client(m_lyricsClient);
}

bool ClientRegistry::loadFrom(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        qCWarning(logInnerTube) << "client registry" << path << "is malformed"
                                << error.errorString();
        return false;
    }

    const QJsonObject root = document.object();
    const QJsonObject clients = root.value(QStringLiteral("clients")).toObject();
    if (clients.isEmpty())
        return false;

    m_clients.clear();
    for (auto it = clients.constBegin(); it != clients.constEnd(); ++it)
        m_clients.insert(it.key(), readClient(it.key(), it.value().toObject()));

    m_streamChain = readChain(root, QStringLiteral("streamChain"));
    m_uploadChain = readChain(root, QStringLiteral("uploadChain"));
    m_metadataClient = root.value(QStringLiteral("metadataClient")).toString();
    m_lyricsClient = root.value(QStringLiteral("lyricsClient")).toString();

    qCInfo(logInnerTube) << "client registry loaded from" << path << m_clients.size() << "clients";
    return true;
}

QStringList ClientRegistry::readChain(const QJsonObject &root, const QString &key)
{
    QStringList chain;
    for (const QJsonValue &value : root.value(key).toArray()) {
        const QString name = value.toString();
        if (m_clients.contains(name))
            chain.append(name);
        else
            qCWarning(logInnerTube) << key << "names an unknown client" << name;
    }
    return chain;
}

}
