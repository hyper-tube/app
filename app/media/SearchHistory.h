#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QStringList>

namespace media {

class SearchHistory : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QStringList queries READ queries NOTIFY queriesChanged)

public:
    explicit SearchHistory(QObject *parent);

    static SearchHistory &instance();
    static SearchHistory *create(QQmlEngine *, QJSEngine *);

    const QStringList &queries() const { return m_queries; }

    void remember(const QString &query);
    Q_INVOKABLE void forget(const QString &query);

Q_SIGNALS:
    void queriesChanged();

private:
    void save() const;

    QStringList m_queries;
};

}
