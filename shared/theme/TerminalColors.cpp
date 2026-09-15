#include "TerminalColors.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

const QString kSpecialGroup = QStringLiteral("special");
const QString kColorsGroup = QStringLiteral("colors");
const QString kBackgroundKey = QStringLiteral("background");

const QStringList kAccentKeys {
    QStringLiteral("color4"),
    QStringLiteral("color5"),
    QStringLiteral("color1"),
    QStringLiteral("color2"),
};

QColor firstOf(const QJsonObject &colors, const QStringList &keys)
{
    for (const QString &key : keys) {
        const QColor color(colors.value(key).toString());
        if (color.isValid())
            return color;
    }
    return {};
}

}

namespace theme {

TerminalColors::TerminalColors(const QString &id, const QString &name, const QString &path,
                               QObject *parent)
    : ColorFile(id, name, path, parent)
{
}

void TerminalColors::read(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        clear();
        return;
    }

    const QJsonObject json = QJsonDocument::fromJson(file.readAll()).object();
    const QColor accent = firstOf(json.value(kColorsGroup).toObject(), kAccentKeys);
    if (!accent.isValid()) {
        clear();
        return;
    }

    const QColor background(json.value(kSpecialGroup).toObject().value(kBackgroundKey).toString());
    adopt({}, accent, schemeFor(background));
}

}
