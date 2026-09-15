#include "MaterialColors.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

const QString kBackgroundRole = QStringLiteral("background");
const QString kPrimaryRole = QStringLiteral("primary");
const QString kSurfaceRole = QStringLiteral("surface");

}

namespace theme {

MaterialColors::MaterialColors(const QString &id, const QString &name, const QString &path,
                               QObject *parent)
    : ColorFile(id, name, path, parent)
{
}

void MaterialColors::read(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        clear();
        return;
    }

    const QJsonObject json = QJsonDocument::fromJson(file.readAll()).object();

    Roles roles;
    for (auto entry = json.constBegin(); entry != json.constEnd(); ++entry) {
        const QColor color(entry.value().toString());
        if (color.isValid())
            roles.insert(entry.key(), color);
    }

    const QColor background = roles.value(kBackgroundRole, roles.value(kSurfaceRole));
    if (!roles.contains(kPrimaryRole) || !background.isValid()) {
        clear();
        return;
    }

    adopt(roles, roles.value(kPrimaryRole), schemeFor(background));
}

}
