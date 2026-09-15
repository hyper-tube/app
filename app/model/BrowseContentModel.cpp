#include "BrowseContentModel.h"

#include <algorithm>
#include <utility>

namespace {

enum Role {
    Type = Qt::UserRole,
    Section,
    ItemIndex,
    Entry,
    Entries,
    Cells,
    Title,
    Subtitle,
    More,
    HasMore,
    Busy,
    Lead,
    Description,
    Message,
};

const QString kRanked = QStringLiteral("ranked");
const QString kGrid = QStringLiteral("grid");
const QString kChips = QStringLiteral("chips");
const QString kMoods = QStringLiteral("moods");
const QString kDestinations = QStringLiteral("destinations");
const QString kSong = QStringLiteral("song");
const QString kEpisode = QStringLiteral("episode");
const QString kCard = QStringLiteral("card");
const QString kMessage = QStringLiteral("message");
const QString kDescription = QStringLiteral("description");
const QString kHeading = QStringLiteral("heading");
const QString kLibrary = QStringLiteral("library");
const QString kCategory = QStringLiteral("category");

constexpr int kSampledCells = 12;

bool holdsTracks(const model::ItemModel *items)
{
    const int sampled = qMin(items->rowCount(), kSampledCells);
    if (sampled == 0)
        return false;

    int playable = 0;
    for (int cell = 0; cell < sampled; ++cell)
        playable += items->get(cell).playable() ? 1 : 0;
    return playable * 2 > sampled;
}

}

namespace model {

BrowseContentModel::BrowseContentModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void BrowseContentModel::setPage(BrowseModel *page)
{
    if (m_page == page)
        return;
    for (const auto &connection : std::as_const(m_connections))
        disconnect(connection);
    m_connections.clear();
    m_observed.clear();
    beginResetModel();
    m_page = page;
    observeItems();
    m_rows = layout();
    m_sectionStates = sectionStates();
    endResetModel();
    m_matches = int(std::count_if(m_rows.begin(), m_rows.end(), listed));
    if (page) {
        m_connections.append(
            connect(page, &BrowseModel::stateChanged, this, &BrowseContentModel::sync));
        m_connections.append(
            connect(page, &BrowseModel::filterChanged, this, &BrowseContentModel::sync));
        m_connections.append(
            connect(page, &QAbstractItemModel::rowsInserted, this, &BrowseContentModel::sync));
        m_connections.append(
            connect(page, &QAbstractItemModel::modelReset, this, &BrowseContentModel::sync));
        m_connections.append(
            connect(page, &QAbstractItemModel::dataChanged, this, &BrowseContentModel::sync));
        m_connections.append(connect(page, &QObject::destroyed, this, [this] {
            beginResetModel();
            m_rows.clear();
            m_sectionStates.clear();
            m_matches = 0;
            endResetModel();
            Q_EMIT pageChanged();
        }));
    }
    Q_EMIT pageChanged();
}

void BrowseContentModel::setColumns(int columns)
{
    columns = qMax(1, columns);
    if (m_columns == columns)
        return;
    m_columns = columns;
    sync();
    Q_EMIT columnsChanged();
}

int BrowseContentModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

QVariant BrowseContentModel::sectionData(int section, const QByteArray &role) const
{
    if (!m_page)
        return {};
    return m_page->data(m_page->index(section), m_page->roleNames().key(role));
}

bool BrowseContentModel::spans(const Row &row) const
{
    return row.type == kRanked || row.type == kChips || row.type == kMoods
        || row.type == kDestinations || row.type == kCard;
}

bool BrowseContentModel::listed(const Row &row)
{
    return row.type == kSong || row.type == kEpisode;
}

int BrowseContentModel::firstCell(const Row &row) const
{
    return spans(row) ? 0 : row.item;
}

int BrowseContentModel::cellLimit(const Row &row, const ItemModel *items) const
{
    if (spans(row))
        return items ? items->rowCount() : 0;
    return row.item + (row.type == kGrid ? m_columns : 1);
}

QString BrowseContentModel::navigationStyle(const ItemModel *items) const
{
    bool striped = false;
    bool glyphed = false;
    for (const Item &item : items->items()) {
        striped = striped || item.striped;
        glyphed = glyphed || !item.glyph.isEmpty();
    }
    if (glyphed)
        return kDestinations;
    return striped ? kMoods : kChips;
}

QVariant BrowseContentModel::data(const QModelIndex &index, int role) const
{
    if (!m_page || !index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return {};
    const Row &row = m_rows.at(index.row());
    auto *items = sectionData(row.section, "entries").value<ItemModel *>();
    switch (role) {
    case Type: return row.type;
    case Section: return row.section;
    case ItemIndex: return row.item;
    case Entry:
        return QVariant::fromValue(row.identity != 0 ? row.snapshot
                                       : items       ? items->get(row.item)
                                                     : Item());
    case Entries: return QVariant::fromValue(items);
    case Cells: {
        QVariantList cells;
        if (items && (row.type == kGrid || spans(row))) {
            for (int i = firstCell(row); i < qMin(cellLimit(row, items), items->rowCount()); ++i)
                cells.append(QVariant::fromValue(items->get(i)));
        }
        return cells;
    }
    case Title: return sectionData(row.section, "sectionTitle");
    case Subtitle: return sectionData(row.section, "sectionSubtitle");
    case More: return sectionData(row.section, "moreDestination");
    case HasMore: return sectionData(row.section, "moreAvailable");
    case Busy: return sectionData(row.section, "busy");
    case Lead: return sectionData(row.section, "lead");
    case Description: return sectionData(row.section, "sectionDescription");
    case Message: return sectionData(row.section, "sectionMessage");
    default: return {};
    }
}

QHash<int, QByteArray> BrowseContentModel::roleNames() const
{
    return {{Type, "rowType"},
            {Section, "sectionIndex"},
            {ItemIndex, "itemIndex"},
            {Entry, "entry"},
            {Entries, "entries"},
            {Cells, "cells"},
            {Title, "sectionTitle"},
            {Subtitle, "sectionSubtitle"},
            {More, "moreDestination"},
            {HasMore, "moreAvailable"},
            {Busy, "busy"},
            {Lead, "lead"},
            {Description, "sectionDescription"},
            {Message, "sectionMessage"}};
}

void BrowseContentModel::observeItems()
{
    if (!m_page)
        return;
    for (int section = 0; section < m_page->rowCount(); ++section) {
        auto *items = sectionData(section, "entries").value<ItemModel *>();
        if (!items || m_observed.contains(items))
            continue;
        m_observed.insert(items);
        m_connections.append(
            connect(items, &ItemModel::countChanged, this, &BrowseContentModel::sync));
        m_connections.append(
            connect(items, &QAbstractItemModel::rowsMoved, this, &BrowseContentModel::sync));
        m_connections.append(
            connect(items, &QAbstractItemModel::dataChanged, this,
                    [this, section, items](const QModelIndex &first, const QModelIndex &last) {
            for (int row = 0; row < m_rows.size(); ++row) {
                Row &entry = m_rows[row];
                if (entry.section == section && firstCell(entry) <= last.row()
                    && cellLimit(entry, items) > first.row()) {
                    entry.snapshot = items->get(entry.item);
                    Q_EMIT dataChanged(index(row), index(row), {Entry, Cells});
                }
            }
        }));
        m_connections.append(
            connect(items, &QObject::destroyed, this, [this, items] { m_observed.remove(items); }));
    }
}

QList<BrowseContentModel::Row> BrowseContentModel::layout() const
{
    QList<Row> rows;
    for (int section = 0; m_page && section < m_page->rowCount(); ++section) {
        auto *items = sectionData(section, "entries").value<ItemModel *>();
        if (!items)
            continue;
        const bool horizontal = sectionData(section, "horizontal").toBool();
        const bool navigation = sectionData(section, "navigation").toBool();
        const bool catalogue = m_page->kind() == kLibrary || m_page->kind() == kCategory;
        const bool listedTracks = m_page->kind() == kCategory && holdsTracks(items);
        const bool grid = horizontal && catalogue && !listedTracks;
        const bool ranked = sectionData(section, "ranked").toBool();
        if (sectionData(section, "card").toBool()) {
            rows.append({section, -1, kCard});
        } else if (!sectionData(section, "sectionDescription").toString().isEmpty()) {
            rows.append({section, -1, kHeading});
            rows.append({section, -1, kDescription});
        } else if (items->rowCount() == 0
                   && !sectionData(section, "sectionMessage").toString().isEmpty()) {
            rows.append({section, -1, kMessage});
        } else if (navigation) {
            const QString style = navigationStyle(items);
            if (style != kMoods)
                rows.append({section, -1, kHeading});
            rows.append({section, -1, style});
        } else if (ranked && horizontal) {
            rows.append({section, -1, kRanked});
        } else if (horizontal && !catalogue) {
            rows.append({section, -1, QStringLiteral("shelf")});
        } else {
            rows.append({section, -1, kHeading});
            for (int item = 0; item < items->rowCount(); item += grid ? m_columns : 1) {
                const Item entry = grid ? Item() : items->get(item);
                const bool detailed = entry.episode()
                    && (!entry.description.isEmpty() || !entry.progressLabel.isEmpty());
                rows.append({section, item,
                             grid           ? kGrid
                                 : detailed ? kEpisode
                                            : kSong,
                             grid ? 0 : items->identity(item), entry});
            }
        }
        rows.append({section, -1, QStringLiteral("continuation")});
    }
    return rows;
}

QList<BrowseContentModel::SectionState> BrowseContentModel::sectionStates() const
{
    QList<SectionState> states;
    for (int section = 0; m_page && section < m_page->rowCount(); ++section) {
        states.append({sectionData(section, "sectionTitle").toString(),
                       sectionData(section, "sectionSubtitle").toString(),
                       sectionData(section, "moreDestination").value<Item>().browseId,
                       sectionData(section, "moreAvailable").toBool(),
                       sectionData(section, "busy").toBool()});
    }
    return states;
}

void BrowseContentModel::sync()
{
    observeItems();
    const QList<Row> rows = layout();
    const QSet<Row> retained(rows.cbegin(), rows.cend());
    const int matches = int(std::count_if(rows.begin(), rows.end(), listed));
    if (m_matches != matches) {
        m_matches = matches;
        Q_EMIT pageChanged();
    }
    for (int row = m_rows.size() - 1; row >= 0; --row) {
        if (retained.contains(m_rows.at(row)))
            continue;
        beginRemoveRows({}, row, row);
        m_rows.removeAt(row);
        endRemoveRows();
    }
    for (int row = 0; row < rows.size(); ++row) {
        const int existing = m_rows.indexOf(rows.at(row), row);
        if (existing < 0) {
            beginInsertRows({}, row, row);
            m_rows.insert(row, rows.at(row));
            endInsertRows();
        } else if (existing != row) {
            beginMoveRows({}, existing, existing, {}, row);
            m_rows.move(existing, row);
            endMoveRows();
        }
        m_rows[row] = rows.at(row);
    }

    const QList<SectionState> states = sectionStates();
    const QList<SectionState> previous = std::exchange(m_sectionStates, states);
    for (int row = 0; row < m_rows.size(); ++row) {
        const Row &entry = m_rows.at(row);
        QList<int> roles {Entry, Entries, ItemIndex};
        if (previous.value(entry.section) != states.value(entry.section))
            roles.append({Title, Subtitle, More, HasMore, Busy});
        if (entry.type == kGrid || spans(entry))
            roles.append(Cells);
        if (!roles.isEmpty())
            Q_EMIT dataChanged(index(row), index(row), roles);
    }
}

}
