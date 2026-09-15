#pragma once

#include "media/Track.h"
#include "plugin/Plugin.h"

#include <QElapsedTimer>
#include <QJsonObject>
#include <QQmlEngine>
#include <QString>

class QTimer;

namespace plugin::discord {

class DiscordIpc;

class DiscordPresence : public plugin::Plugin
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool sharing READ sharing WRITE setSharing NOTIFY presenceChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY presenceChanged)
    Q_PROPERTY(media::Track shown READ shown NOTIFY presenceChanged)

public:
    explicit DiscordPresence(QObject *parent);

    static DiscordPresence &instance();
    static DiscordPresence *create(QQmlEngine *, QJSEngine *);

    PluginInfo info() const override;

    bool sharing() const;
    bool connected() const;
    media::Track shown() const;

    void setSharing(bool sharing);

    Q_INVOKABLE void reconnect();

Q_SIGNALS:
    void presenceChanged();

protected:
    QList<PluginSetting> schema() const override;
    void start() override;
    void stop() override;
    void valueChanged(const QString &key) override;
    void restate() override;

private:
    void refresh();
    void schedule();
    void publishNow();
    void publish(bool force);
    void retryLater();
    void noteFailure(const QString &message);
    QJsonObject activity() const;

    DiscordIpc *m_ipc;
    QTimer *m_throttle;
    QTimer *m_retry;
    QTimer *m_reassert;
    QElapsedTimer m_sent;
    QJsonObject m_published;
    QString m_failure;
    int m_backoff = 0;
    bool m_refused = false;
};

}
