#pragma once

#include <QColor>

#include <functional>

class QObject;

namespace theme::apple {

QColor accentColor();
Qt::ColorScheme colorScheme();
void observeAppearance(QObject *owner, std::function<void()> notify);

}
