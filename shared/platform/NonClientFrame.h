#pragma once

#include "WindowFrame.h"

#include <QAbstractNativeEventFilter>
#include <QRect>

#include <windows.h>

namespace platform {

class NonClientFrame : public WindowFrame, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    NonClientFrame(QWindow *window, QObject *parent);
    ~NonClientFrame() override;

    void setSnapRegion(const QRect &region) override;
    void showSystemMenu() override;

    bool nativeEventFilter(const QByteArray &type, void *message, qintptr *result) override;

private:
    bool trimFrame(const MSG *message, qintptr *result);
    bool hitTest(const MSG *message, qintptr *result);
    bool resizable() const;
    bool trackSnapButton(const MSG *message, qintptr *result);

    HWND m_handle = nullptr;
    QRect m_snapRegion;
    bool m_snapHovered = false;
    bool m_snapPressed = false;
};

}
