#pragma once

#include <QJsonObject>
#include <QString>

namespace innertube {

struct Client;

class ContextBuilder
{
public:
    ContextBuilder();

    QJsonObject build(const Client &client, const QString &visitorData,
                      const QString &dataSyncId) const;

private:
    QString m_country;
};

}
