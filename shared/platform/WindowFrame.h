#pragma once

#include <QObject>
#include <QRect>

class QWindow;

namespace platform {

class WindowFrame : public QObject
{
    Q_OBJECT

public:
    static WindowFrame *create(QWindow *window, QObject *parent);

    WindowFrame(QWindow *window, QObject *parent);

    QWindow *window() const { return m_window; }

    virtual int titleAreaHeight() const { return 0; }
    virtual int leadingInset() const { return 0; }
    virtual void setSnapRegion(const QRect &region);
    virtual void showSystemMenu();

Q_SIGNALS:
    void snapHoveredChanged(bool hovered);
    void snapPressedChanged(bool pressed);
    void snapTriggered();

private:
    QWindow *m_window;
};

}
