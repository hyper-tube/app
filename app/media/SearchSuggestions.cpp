#include "SearchSuggestions.h"

#include "innertube/parsers/RendererReader.h"
#include "library/OfflineSearch.h"
#include "net/Connectivity.h"

#include <QJsonArray>
#include <QPointer>

namespace {

constexpr int kSuggestionLimit = 8;

void collect(const QJsonValue &value, QStringList &result)
{
    if (result.size() >= kSuggestionLimit)
        return;
    if (value.isArray()) {
        for (const QJsonValue &child : value.toArray())
            collect(child, result);
    } else if (value.isObject()) {
        const QJsonObject object = value.toObject();
        const QJsonObject suggestion =
            object.value(QStringLiteral("searchSuggestionRenderer")).toObject();
        QString query =
            innertube::parsers::findFirst(suggestion.value(QStringLiteral("navigationEndpoint")),
                                          QStringLiteral("query"))
                .toString();
        if (query.isEmpty())
            query = innertube::parsers::readText(suggestion.value(QStringLiteral("suggestion")));
        if (!query.isEmpty() && !result.contains(query))
            result.append(query);
        for (auto it = object.begin(); it != object.end(); ++it)
            collect(it.value(), result);
    }
}

}

namespace media {

SearchSuggestions::SearchSuggestions(QObject *parent)
    : QObject(parent)
    , m_endpoints(innertube::Session::instance())
{
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(180);
    connect(&m_debounce, &QTimer::timeout, this, &SearchSuggestions::fetch);
}

void SearchSuggestions::setQuery(const QString &query)
{
    const QString trimmed = query.trimmed();
    if (m_query == trimmed)
        return;
    m_query = trimmed;
    ++m_generation;
    m_debounce.stop();
    m_suggestions.clear();
    Q_EMIT queryChanged();
    Q_EMIT suggestionsChanged();
    if (!trimmed.isEmpty())
        m_debounce.start();
}

void SearchSuggestions::fetch()
{
    if (!net::Connectivity::instance().online()) {
        m_suggestions = library::offlineSearch::suggestions(m_query, kSuggestionLimit);
        Q_EMIT suggestionsChanged();
        return;
    }
    const quint64 generation = m_generation;
    const QPointer<SearchSuggestions> guard(this);
    m_endpoints.action(QStringLiteral("music/get_search_suggestions"),
                       {{QStringLiteral("input"), m_query}},
                       [guard, generation](const innertube::Reply &reply) {
        if (!guard || generation != guard->m_generation || !reply.ok())
            return;
        QStringList suggestions;
        collect(reply.json, suggestions);
        guard->m_suggestions = suggestions;
        Q_EMIT guard->suggestionsChanged();
    });
}

}
