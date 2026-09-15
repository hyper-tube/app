#pragma once

#include <QByteArray>
#include <QObject>
#include <QPointer>

class QQmlEngine;

namespace media {
class Browser;
}

namespace diagnostics {

class StateObserver : public QObject
{
    Q_OBJECT

public:
    explicit StateObserver(QObject *parent = nullptr);

    void bind(QQmlEngine &engine);
    void publish() const;

    const QByteArray &page() const { return m_page; }

private:
    void followPage();
    void followAccount();
    void followSetting(const char *setting) const;
    void publishPlayback() const;
    void publishPlugins() const;

    QPointer<media::Browser> m_browser;
    QByteArray m_page = QByteArrayLiteral("none");
    int m_accountStatus = -1;
    bool m_bound = false;
};

}
