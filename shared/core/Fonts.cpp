#include "Fonts.h"

#include "Logging.h"

#include <QFontDatabase>
#include <QStringList>

namespace {

const QStringList kBundled {
    QStringLiteral(":/fonts/RobotoFlex.ttf"),
    QStringLiteral(":/fonts/MaterialSymbolsRounded.ttf"),
};

}

namespace core::fonts {

void install()
{
    for (const QString &path : kBundled) {
        const int id = QFontDatabase::addApplicationFont(path);
        if (id < 0) {
            qCWarning(logTheme) << "bundled font rejected" << path;
            continue;
        }
        qCDebug(logTheme) << "bundled font" << QFontDatabase::applicationFontFamilies(id);
    }
}

}
