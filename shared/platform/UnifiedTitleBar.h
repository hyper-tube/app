#pragma once

#include "WindowFrame.h"

namespace platform {

class UnifiedTitleBar : public WindowFrame
{
    Q_OBJECT

public:
    UnifiedTitleBar(QWindow *window, QObject *parent);

    int titleAreaHeight() const override { return m_titleAreaHeight; }
    int leadingInset() const override { return m_leadingInset; }

private:
    int m_titleAreaHeight = 0;
    int m_leadingInset = 0;
};

}
