#include "PlasmaColors.h"

#include <QSettings>
#include <QStringList>

namespace {

constexpr int kChannelCount = 3;

const QString kAccentKey = QStringLiteral("General/AccentColor");
const QString kSelectionKey = QStringLiteral("Colors:Selection/BackgroundNormal");
const QString kWindowKey = QStringLiteral("Colors:Window/BackgroundNormal");

QColor channelsToColor(const QVariant &stored)
{
    const QStringList channels = stored.toStringList();
    if (channels.size() != kChannelCount)
        return {};

    int values[kChannelCount] = {};
    for (int index = 0; index < kChannelCount; ++index) {
        bool parsed = false;
        values[index] = channels.at(index).trimmed().toInt(&parsed);
        if (!parsed)
            return {};
    }
    return QColor::fromRgb(values[0], values[1], values[2]);
}

}

namespace theme {

PlasmaColors::PlasmaColors(const QString &path, QObject *parent)
    : ColorFile(QStringLiteral("plasma"), QStringLiteral("KDE Plasma"), path, parent)
{
}

void PlasmaColors::read(const QString &path)
{
    const QSettings globals(path, QSettings::IniFormat);
    if (globals.status() != QSettings::NoError) {
        clear();
        return;
    }

    QColor accent = channelsToColor(globals.value(kAccentKey));
    if (!accent.isValid())
        accent = channelsToColor(globals.value(kSelectionKey));
    if (!accent.isValid()) {
        clear();
        return;
    }

    adopt({}, accent, schemeFor(channelsToColor(globals.value(kWindowKey))));
}

}
