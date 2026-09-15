#pragma once

#include "Plugin.h"

#include <QAbstractListModel>
#include <QList>
#include <QQmlEngine>
#include <QString>
#include <QStringList>

namespace plugin {

class PluginRegistry : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY changed)
    Q_PROPERTY(int total READ total CONSTANT)
    Q_PROPERTY(QStringList cards READ cards NOTIFY cardsChanged)

public:
    explicit PluginRegistry(QObject *parent);

    static PluginRegistry &instance();
    static PluginRegistry *create(QQmlEngine *, QJSEngine *);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    const QList<Plugin *> &plugins() const { return m_plugins; }
    const QString &query() const { return m_query; }
    int total() const { return int(m_plugins.size()); }
    QStringList cards() const;

    void setQuery(const QString &query);
    void restore();
    void shutDown();

    Q_INVOKABLE plugin::Plugin *find(const QString &id) const;
    Q_INVOKABLE void openPage(const QString &id);

Q_SIGNALS:
    void changed();
    void queryChanged();
    void cardsChanged();
    void pageRequested(const QString &id);

private:
    void refilter();
    bool matches(const Plugin *plugin) const;

    QList<Plugin *> m_plugins;
    QList<Plugin *> m_visible;
    QString m_query;
};

}
