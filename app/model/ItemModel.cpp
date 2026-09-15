#include "ItemModel.h"

namespace model {

ItemModel::ItemModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ItemModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant ItemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()
        || role != Qt::UserRole)
        return {};
    return QVariant::fromValue(m_items.at(index.row()));
}

QHash<int, QByteArray> ItemModel::roleNames() const
{
    return {{Qt::UserRole, "entry"}};
}

void ItemModel::append(const QList<Item> &items)
{
    if (items.isEmpty())
        return;
    const int first = m_items.size();
    beginInsertRows({}, first, first + items.size() - 1);
    m_items.append(items);
    for (int i = 0; i < items.size(); ++i)
        m_identities.append(m_nextIdentity++);
    endInsertRows();
    Q_EMIT countChanged();
}

quint64 ItemModel::identity(int index) const
{
    return m_identities.value(index);
}

bool ItemModel::move(int from, int to)
{
    if (from < 0 || to < 0 || from >= m_items.size() || to >= m_items.size() || from == to)
        return false;
    beginMoveRows({}, from, from, {}, to > from ? to + 1 : to);
    m_items.move(from, to);
    m_identities.move(from, to);
    endMoveRows();
    return true;
}

int ItemModel::entryIndex(const QString &setVideoId) const
{
    if (setVideoId.isEmpty())
        return -1;
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items.at(i).actions.setVideoId == setVideoId)
            return i;
    }
    return -1;
}

void ItemModel::applyRating(const QString &videoId, int rating)
{
    for (int row = 0; row < m_items.size(); ++row) {
        Item &item = m_items[row];
        if (item.track.videoId != videoId)
            continue;
        item.track.liked = rating > 0;
        item.track.disliked = rating < 0;
        item.track.ratingKnown = true;
        Q_EMIT dataChanged(index(row), index(row));
    }
}

void ItemModel::applyPinned(const QString &key, bool pinned)
{
    for (int row = 0; row < m_items.size(); ++row) {
        Item &item = m_items[row];
        if (item.key() != key || item.pinned == pinned)
            continue;
        item.pinned = pinned;
        Q_EMIT dataChanged(index(row), index(row));
    }
}

void ItemModel::applyInLibrary(const QString &key, bool inLibrary)
{
    for (int row = 0; row < m_items.size(); ++row) {
        Item &item = m_items[row];
        if (item.key() != key || item.actions.inLibrary == inLibrary)
            continue;
        item.actions.inLibrary = inLibrary;
        Q_EMIT dataChanged(index(row), index(row));
    }
}

void ItemModel::applyLater(const QString &videoId, bool later)
{
    for (int row = 0; row < m_items.size(); ++row) {
        Item &item = m_items[row];
        if (item.track.videoId != videoId || item.actions.later == later)
            continue;
        item.actions.later = later;
        Q_EMIT dataChanged(index(row), index(row));
    }
}

void ItemModel::applyPlayed(const QString &videoId, bool played)
{
    for (int row = 0; row < m_items.size(); ++row) {
        Item &item = m_items[row];
        if (item.track.videoId != videoId || item.played == played)
            continue;
        item.played = played;
        if (!played)
            item.progress = 0;
        Q_EMIT dataChanged(index(row), index(row));
    }
}

void ItemModel::removeEntries(const QStringList &setVideoIds)
{
    for (int row = m_items.size() - 1; row >= 0; --row) {
        if (!setVideoIds.contains(m_items.at(row).actions.setVideoId))
            continue;
        beginRemoveRows({}, row, row);
        m_items.remove(row);
        m_identities.remove(row);
        endRemoveRows();
    }
    Q_EMIT countChanged();
}

bool ItemModel::matches(int index, const QString &needle) const
{
    if (needle.isEmpty())
        return true;
    const Item item = get(index);
    return item.title.contains(needle, Qt::CaseInsensitive)
        || item.subtitle.contains(needle, Qt::CaseInsensitive)
        || item.track.artist.contains(needle, Qt::CaseInsensitive)
        || item.track.album.contains(needle, Qt::CaseInsensitive);
}

QList<media::Track> ItemModel::tracksAt(const QList<int> &rows) const
{
    QList<media::Track> result;
    result.reserve(rows.size());
    for (int const row : rows) {
        const Item item = get(row);
        if (item.playable())
            result.append(item.track);
    }
    return result;
}

Item ItemModel::get(int index) const
{
    return index >= 0 && index < m_items.size() ? m_items.at(index) : Item();
}

QList<media::Track> ItemModel::tracks() const
{
    QList<media::Track> result;
    for (const Item &item : m_items) {
        if (item.playable())
            result.append(item.track);
    }
    return result;
}

int ItemModel::trackIndex(int index) const
{
    if (!get(index).playable())
        return -1;
    int result = 0;
    for (int i = 0; i < index; ++i) {
        if (m_items.at(i).playable())
            ++result;
    }
    return result;
}

}
