#include "Clipboard.h"

#include <QClipboard>
#include <QGuiApplication>

namespace platform {

Clipboard::Clipboard(QObject *parent)
    : QObject(parent)
{
}

bool Clipboard::copy(const QString &text)
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    if (text.isEmpty() || !clipboard)
        return false;
    clipboard->setText(text);
    return true;
}

QString Clipboard::text() const
{
    const QClipboard *clipboard = QGuiApplication::clipboard();
    return clipboard ? clipboard->text() : QString();
}

}
