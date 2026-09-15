#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QRect>
#include <QTimer>

class QQuickWindow;

namespace platform {

class WindowState : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int initialX READ initialX CONSTANT)
    Q_PROPERTY(int initialY READ initialY CONSTANT)
    Q_PROPERTY(int initialWidth READ initialWidth CONSTANT)
    Q_PROPERTY(int initialHeight READ initialHeight CONSTANT)

public:
    explicit WindowState(QObject *parent);

    static WindowState &instance();
    static WindowState *create(QQmlEngine *, QJSEngine *);

    void bind(QQuickWindow *window);

    int initialX() const { return m_restored.x(); }
    int initialY() const { return m_restored.y(); }
    int initialWidth() const { return m_restored.width(); }
    int initialHeight() const { return m_restored.height(); }

private:
    void record();
    void save();
    void keepOnScreen();
    void adoptRestored();

    QTimer m_saveTimer;
    QQuickWindow *m_window = nullptr;
    QRect m_restored;
    QRect m_current;
    bool m_maximized = false;
};

}
