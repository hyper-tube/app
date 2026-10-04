#include "ShortcutKeys.h"

#include <QLatin1StringView>

#include <array>
#include <optional>

namespace {

struct ScanCodeRow
{
    quint32 first;
    QLatin1StringView keys;
};

constexpr std::array kUsLayoutRows {
    ScanCodeRow {0x02, QLatin1StringView("1234567890-=")},
    ScanCodeRow {0x10, QLatin1StringView("QWERTYUIOP[]")},
    ScanCodeRow {0x1E, QLatin1StringView("ASDFGHJKL;'`")},
    ScanCodeRow {0x2B, QLatin1StringView("\\ZXCVBNM,./")},
};

std::optional<quint32> setOneScanCode(quint32 nativeScanCode)
{
#if defined(Q_OS_WIN)
    return nativeScanCode;
#elif defined(Q_OS_MACOS)
    Q_UNUSED(nativeScanCode)
    return std::nullopt;
#else
    constexpr quint32 kXkbKeycodeOffset = 8;
    if (nativeScanCode <= kXkbKeycodeOffset)
        return std::nullopt;
    return nativeScanCode - kXkbKeycodeOffset;
#endif
}

int usLayoutKey(quint32 scanCode)
{
    for (const ScanCodeRow &row : kUsLayoutRows) {
        if (scanCode >= row.first && scanCode - row.first < quint32(row.keys.size()))
            return row.keys.at(qsizetype(scanCode - row.first)).unicode();
    }
    return 0;
}

bool isLatinLetterOrDigit(int key)
{
    return (key >= Qt::Key_A && key <= Qt::Key_Z) || (key >= Qt::Key_0 && key <= Qt::Key_9);
}

}

namespace platform {

ShortcutKeys::ShortcutKeys(QObject *parent)
    : QObject(parent)
{
}

int ShortcutKeys::resolve(int key, quint32 nativeScanCode)
{
    if (isLatinLetterOrDigit(key))
        return key;
    const std::optional<quint32> scanCode = setOneScanCode(nativeScanCode);
    if (!scanCode)
        return key;
    const int physical = usLayoutKey(*scanCode);
    return physical != 0 ? physical : key;
}

}
