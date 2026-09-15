#pragma once

#include "innertube/Endpoints.h"

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QString>
#include <QStringList>

namespace library {

class PlaylistTargets : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool open READ open NOTIFY stateChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY stateChanged)
    Q_PROPERTY(QString error READ error NOTIFY stateChanged)
    Q_PROPERTY(bool unreachable READ unreachable NOTIFY stateChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY stateChanged)
    Q_PROPERTY(int trackCount READ trackCount NOTIFY stateChanged)

public:
    explicit PlaylistTargets(QObject *parent);

    static PlaylistTargets &instance();
    static PlaylistTargets *create(QQmlEngine *, QJSEngine *);

    bool open() const { return m_open; }
    bool loading() const { return m_loading; }
    const QString &error() const { return m_error; }
    bool unreachable() const { return m_unreachable; }
    int trackCount() const { return m_videoIds.size(); }
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void show(const QStringList &videoIds);
    Q_INVOKABLE void dismiss();
    Q_INVOKABLE void reload();
    Q_INVOKABLE void retry();
    Q_INVOKABLE void choose(int index);
    Q_INVOKABLE void createWith(const QString &title);

Q_SIGNALS:
    void stateChanged();

private:
    struct Target
    {
        QString playlistId;
        QString title;
        QString subtitle;
        QString artId;
    };

    void accept(const innertube::Reply &reply, quint64 generation);

    innertube::Endpoints m_endpoints;
    QList<Target> m_targets;
    QStringList m_videoIds;
    QString m_error;
    quint64 m_generation = 0;
    bool m_open = false;
    bool m_loading = false;
    bool m_unreachable = false;
};

}
