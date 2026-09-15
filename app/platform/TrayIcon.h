#pragma once

#include <QObject>
#include <QPoint>
#include <QQmlEngine>
#include <QRect>
#include <QSystemTrayIcon>
#include <QWindow>

class QAction;
class QMenu;

namespace media {
class PlaybackController;
}

namespace platform {

class TrayIcon : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool styled READ styled CONSTANT)
    Q_PROPERTY(QPoint anchor READ anchor NOTIFY menuRequested)
    Q_PROPERTY(QRect anchorArea READ anchorArea NOTIFY menuRequested)

public:
    explicit TrayIcon(QObject *parent);
    ~TrayIcon() override;

    static TrayIcon &instance();
    static TrayIcon *create(QQmlEngine *, QJSEngine *);
    static bool styled();

    const QPoint &anchor() const { return m_anchor; }
    const QRect &anchorArea() const { return m_anchorArea; }

    Q_INVOKABLE void present(QWindow *menu);
    Q_INVOKABLE void toggleWindow();
    Q_INVOKABLE void quit();

Q_SIGNALS:
    void toggleWindowRequested();
    void quitRequested();
    void menuRequested();

private:
    void buildMenu();
    void openMenu();
    void refresh();
    void retranslate();

    media::PlaybackController &m_controller;
    QSystemTrayIcon m_icon;
    QMenu *m_menu = nullptr;
    QAction *m_playPause = nullptr;
    QAction *m_previous = nullptr;
    QAction *m_next = nullptr;
    QAction *m_window = nullptr;
    QAction *m_quit = nullptr;
    QPoint m_anchor;
    QRect m_anchorArea;
};

}
