#pragma once

#include "Notification.h"

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QStringList>

namespace control {

class ControlCenter : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int count READ rowCount NOTIFY changed)
    Q_PROPERTY(int unreadCount READ unreadCount NOTIFY changed)
    Q_PROPERTY(bool clearable READ clearable NOTIFY changed)
    Q_PROPERTY(Status status READ status NOTIFY changed)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)

public:
    enum Status {
        Quiet,
        Alert,
        Busy,
    };
    Q_ENUM(Status)

    explicit ControlCenter(QObject *parent);

    static ControlCenter &instance();
    static ControlCenter *create(QQmlEngine *, QJSEngine *);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int unreadCount() const;
    bool unread(const QString &id) const;
    bool clearable() const;
    Status status() const;
    double progress() const;

    Q_INVOKABLE void publish(const control::Notification &entry);
    Q_INVOKABLE void retract(const QString &id);

    Q_INVOKABLE void invokeAction(const QString &id);
    Q_INVOKABLE void dismiss(const QString &id);
    Q_INVOKABLE void dismissAll();
    Q_INVOKABLE void markRead();

Q_SIGNALS:
    void changed();
    void progressChanged();
    void arrived();
    void actionInvoked(const QString &id);

private:
    int indexOf(const QString &id) const;
    int placeFor(const Notification &entry) const;
    void remember(const QString &id);

    QList<Notification> m_entries;
    QStringList m_dismissed;
};

}
