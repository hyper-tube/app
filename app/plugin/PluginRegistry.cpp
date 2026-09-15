#include "PluginRegistry.h"

#include "discord/DiscordPresence.h"

#include <QCoreApplication>

namespace plugin {

PluginRegistry::PluginRegistry(QObject *parent)
    : QAbstractListModel(parent)
{
    m_plugins.append(&discord::DiscordPresence::instance());
    for (Plugin const *entry : std::as_const(m_plugins)) {
        connect(entry, &Plugin::enabledChanged, this, &PluginRegistry::cardsChanged);
        connect(entry, &Plugin::retranslated, this, &PluginRegistry::refilter);
    }
    m_visible = m_plugins;
}

PluginRegistry &PluginRegistry::instance()
{
    static auto *registry = new PluginRegistry(QCoreApplication::instance());
    return *registry;
}

PluginRegistry *PluginRegistry::create(QQmlEngine *, QJSEngine *)
{
    QQmlEngine::setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
    return &instance();
}

int PluginRegistry::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_visible.size();
}

QVariant PluginRegistry::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visible.size()
        || role != Qt::UserRole)
        return {};
    return QVariant::fromValue(m_visible.at(index.row()));
}

QHash<int, QByteArray> PluginRegistry::roleNames() const
{
    return {{Qt::UserRole, "plugin"}};
}

QStringList PluginRegistry::cards() const
{
    QStringList sources;
    for (const Plugin *entry : m_plugins) {
        const PluginInfo info = entry->info();
        if (entry->enabled() && !info.cardSource.isEmpty())
            sources.append(info.cardSource);
    }
    return sources;
}

bool PluginRegistry::matches(const Plugin *plugin) const
{
    if (m_query.isEmpty())
        return true;
    const PluginInfo info = plugin->info();
    return info.name.contains(m_query, Qt::CaseInsensitive)
        || info.description.contains(m_query, Qt::CaseInsensitive)
        || info.id.contains(m_query, Qt::CaseInsensitive);
}

void PluginRegistry::refilter()
{
    bool touched = false;
    for (int row = m_visible.size() - 1; row >= 0; --row) {
        if (matches(m_visible.at(row)))
            continue;
        beginRemoveRows({}, row, row);
        m_visible.remove(row);
        endRemoveRows();
        touched = true;
    }

    int row = 0;
    for (Plugin *entry : std::as_const(m_plugins)) {
        if (!matches(entry))
            continue;
        if (row < m_visible.size() && m_visible.at(row) == entry) {
            ++row;
            continue;
        }
        beginInsertRows({}, row, row);
        m_visible.insert(row, entry);
        endInsertRows();
        ++row;
        touched = true;
    }

    if (touched)
        Q_EMIT changed();
}

void PluginRegistry::setQuery(const QString &query)
{
    if (m_query == query)
        return;
    m_query = query;
    refilter();
    Q_EMIT queryChanged();
}

void PluginRegistry::restore()
{
    for (Plugin *entry : std::as_const(m_plugins))
        entry->restore();
    Q_EMIT cardsChanged();
}

void PluginRegistry::shutDown()
{
    for (Plugin *entry : std::as_const(m_plugins))
        entry->shutDown();
}

Plugin *PluginRegistry::find(const QString &id) const
{
    for (Plugin *entry : m_plugins) {
        if (entry->info().id == id)
            return entry;
    }
    return nullptr;
}

void PluginRegistry::openPage(const QString &id)
{
    if (find(id))
        Q_EMIT pageRequested(id);
}

}
