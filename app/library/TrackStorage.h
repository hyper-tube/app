#pragma once

#include "media/Track.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>

namespace library::trackStorage {

QJsonObject encode(const media::Track &track);
media::Track decode(const QJsonObject &object);
QJsonArray encodeCredits(const QList<media::Credit> &credits);
QList<media::Credit> decodeCredits(const QJsonValue &value);

}
