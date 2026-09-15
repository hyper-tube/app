#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QRect>

class QQuickWindow;

namespace platform {

class WindowFrame;

class WindowChrome : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(
        Decorations decorations READ decorations WRITE setDecorations NOTIFY decorationsChanged)
    Q_PROPERTY(bool selectable READ selectable CONSTANT)
    Q_PROPERTY(bool controls READ controls CONSTANT)
    Q_PROPERTY(bool grips READ grips CONSTANT)
    Q_PROPERTY(Qt::WindowFlags windowFlags READ windowFlags CONSTANT)
    Q_PROPERTY(int gripThickness READ gripThickness CONSTANT)
    Q_PROPERTY(int barHeight READ barHeight NOTIFY metricsChanged)
    Q_PROPERTY(int leadingInset READ leadingInset NOTIFY metricsChanged)
    Q_PROPERTY(bool maximized READ maximized NOTIFY maximizedChanged)
    Q_PROPERTY(bool restartPending READ restartPending NOTIFY decorationsChanged)
    Q_PROPERTY(QRect snapRegion READ snapRegion WRITE setSnapRegion NOTIFY snapRegionChanged)
    Q_PROPERTY(bool snapHovered READ snapHovered NOTIFY snapStateChanged)
    Q_PROPERTY(bool snapPressed READ snapPressed NOTIFY snapStateChanged)

public:
    enum Decorations {
        Custom,
        System,
    };
    Q_ENUM(Decorations)

    explicit WindowChrome(QObject *parent);

    static WindowChrome &instance();
    static WindowChrome *create(QQmlEngine *, QJSEngine *);

    void bind(QQuickWindow *window);

    Decorations decorations() const { return m_decorations; }
    bool selectable() const;
    bool controls() const { return m_custom; }
    bool grips() const;
    Qt::WindowFlags windowFlags() const;
    int gripThickness() const;
    int barHeight() const;
    int leadingInset() const;
    bool maximized() const { return m_maximized; }
    bool restartPending() const { return m_decorations != m_applied; }
    const QRect &snapRegion() const { return m_snapRegion; }
    bool snapHovered() const { return m_snapHovered; }
    bool snapPressed() const { return m_snapPressed; }
    bool relaunching() const { return m_relaunching; }

    void setDecorations(Decorations decorations);
    void setSnapRegion(const QRect &region);

    Q_INVOKABLE void startMove();
    Q_INVOKABLE void startResize(int edges);
    Q_INVOKABLE void minimize();
    Q_INVOKABLE void toggleMaximized();
    Q_INVOKABLE void close();
    Q_INVOKABLE void showSystemMenu();
    Q_INVOKABLE void relaunch();

Q_SIGNALS:
    void decorationsChanged();
    void metricsChanged();
    void maximizedChanged();
    void snapRegionChanged();
    void snapStateChanged();

private:
    void adoptVisibility();

    QQuickWindow *m_window = nullptr;
    WindowFrame *m_frame = nullptr;
    Decorations m_decorations = Custom;
    Decorations m_applied = Custom;
    QRect m_snapRegion;
    bool m_custom;
    bool m_maximized = false;
    bool m_snapHovered = false;
    bool m_snapPressed = false;
    bool m_relaunching = false;
};

}
