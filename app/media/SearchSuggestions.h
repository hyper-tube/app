#pragma once

#include "innertube/Endpoints.h"

#include <QObject>
#include <QQmlEngine>
#include <QTimer>

namespace media {

class SearchSuggestions : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(QStringList suggestions READ suggestions NOTIFY suggestionsChanged)

public:
    explicit SearchSuggestions(QObject *parent = nullptr);

    const QString &query() const { return m_query; }
    const QStringList &suggestions() const { return m_suggestions; }
    void setQuery(const QString &query);

Q_SIGNALS:
    void queryChanged();
    void suggestionsChanged();

private:
    void fetch();

    innertube::Endpoints m_endpoints;
    QTimer m_debounce;
    QString m_query;
    QStringList m_suggestions;
    quint64 m_generation = 0;
};

}
