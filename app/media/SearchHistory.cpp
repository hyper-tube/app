#include "SearchHistory.h"

#include <QCoreApplication>
#include <QSettings>

namespace {

constexpr int kHistoryLimit = 8;

const QString kHistoryKey = QStringLiteral("search/history");

}

namespace media {

SearchHistory::SearchHistory(QObject *parent)
    : QObject(parent)
{
    const QSettings settings;
    for (const QString &query : settings.value(kHistoryKey).toStringList()) {
        const QString trimmed = query.trimmed();
        if (!trimmed.isEmpty() && !m_queries.contains(trimmed, Qt::CaseInsensitive))
            m_queries.append(trimmed);
    }
    m_queries = m_queries.mid(0, kHistoryLimit);
}

SearchHistory &SearchHistory::instance()
{
    static auto *history = new SearchHistory(QCoreApplication::instance());
    return *history;
}

SearchHistory *SearchHistory::create(QQmlEngine *, QJSEngine *)
{
    QQmlEngine::setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
    return &instance();
}

void SearchHistory::remember(const QString &query)
{
    const QString trimmed = query.trimmed();
    if (trimmed.isEmpty())
        return;
    QStringList kept {trimmed};
    for (const QString &previous : std::as_const(m_queries)) {
        if (kept.size() < kHistoryLimit && previous.compare(trimmed, Qt::CaseInsensitive) != 0)
            kept.append(previous);
    }
    if (kept == m_queries)
        return;
    m_queries = std::move(kept);
    save();
    Q_EMIT queriesChanged();
}

void SearchHistory::forget(const QString &query)
{
    if (m_queries.removeAll(query) == 0)
        return;
    save();
    Q_EMIT queriesChanged();
}

void SearchHistory::save() const
{
    QSettings settings;
    settings.setValue(kHistoryKey, m_queries);
}

}
