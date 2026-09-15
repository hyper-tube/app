#include "ContextBuilder.h"

#include "ClientRegistry.h"
#include "core/Localization.h"

#include <QJsonArray>
#include <QLocale>

namespace {

const QString kDefaultCountry = QStringLiteral("US");

QString systemCountry()
{
    const QLocale::Territory territory = QLocale::system().territory();
    if (territory == QLocale::AnyTerritory)
        return kDefaultCountry;

    const QString code = QLocale::territoryToCode(territory);
    return code.size() == 2 ? code : kDefaultCountry;
}

void insertIfSet(QJsonObject &object, const QString &key, const QString &value)
{
    if (!value.isEmpty())
        object.insert(key, value);
}

}

namespace innertube {

ContextBuilder::ContextBuilder()
    : m_country(systemCountry())
{
}

QJsonObject ContextBuilder::build(const Client &client, const QString &visitorData,
                                  const QString &dataSyncId) const
{
    QJsonObject clientObject;
    clientObject.insert(QStringLiteral("clientName"), client.name);
    clientObject.insert(QStringLiteral("clientVersion"), client.version);
    insertIfSet(clientObject, QStringLiteral("osName"), client.osName);
    insertIfSet(clientObject, QStringLiteral("osVersion"), client.osVersion);
    insertIfSet(clientObject, QStringLiteral("deviceMake"), client.deviceMake);
    insertIfSet(clientObject, QStringLiteral("deviceModel"), client.deviceModel);
    insertIfSet(clientObject, QStringLiteral("androidSdkVersion"), client.androidSdkVersion);
    clientObject.insert(QStringLiteral("gl"), m_country);
    clientObject.insert(QStringLiteral("hl"), core::Localization::instance().resolved());
    insertIfSet(clientObject, QStringLiteral("visitorData"), visitorData);

    QJsonObject request;
    request.insert(QStringLiteral("internalExperimentFlags"), QJsonArray());
    request.insert(QStringLiteral("useSsl"), true);

    QJsonObject user;
    user.insert(QStringLiteral("lockedSafetyMode"), false);
    insertIfSet(user, QStringLiteral("onBehalfOfUser"), dataSyncId);

    QJsonObject context;
    context.insert(QStringLiteral("client"), clientObject);
    context.insert(QStringLiteral("request"), request);
    context.insert(QStringLiteral("user"), user);
    return context;
}

}
