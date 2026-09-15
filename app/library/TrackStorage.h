#pragma once

#include "media/Track.h"

#include <QJsonObject>

namespace library::trackStorage {

QJsonObject encode(const media::Track &track);
media::Track decode(const QJsonObject &object);

}
