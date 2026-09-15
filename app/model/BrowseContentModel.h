#pragma once

#include "BrowseModel.h"

#include <QAbstractListModel>
#include <QPointer>

namespace model {

class BrowseContentModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(model::BrowseModel *page READ page WRITE setPage NOTIFY pageChanged)
    Q_PROPERTY(int columns READ columns WRITE setColumns NOTIFY columnsChanged)
    Q_PROPERTY(int matchCount READ matchCount NOTIFY pageChanged)

public:
    explicit BrowseContentModel(QObject *parent = nullptr);

    BrowseModel *page() const { return m_page; }
    int columns() const { return m_columns; }
    int matchCount() const { return m_matches; }
    void setPage(BrowseModel *page);
    void setColumns(int columns);
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

Q_SIGNALS:
    void pageChanged();
    void columnsChanged();

private:
    struct Row
    {
        int section;
        int item;
        QString type;

        quint64 identity = 0;
        Item snapshot;

        bool operator==(const Row &other) const
        {
            return section == other.section && type == other.type && identity == other.identity
                && (identity != 0 || item == other.item);
        }

        friend size_t qHash(const Row &row, size_t seed = 0)
        {
            return qHashMulti(seed, row.section, row.type, row.identity,
                              row.identity != 0 ? 0 : row.item);
        }
    };

    struct SectionState
    {
        QString title;
        QString subtitle;
        QString moreId;
        bool hasMore = false;
        bool busy = false;

        bool operator==(const SectionState &) const = default;
    };

    QVariant sectionData(int section, const QByteArray &role) const;
    QString navigationStyle(const ItemModel *items) const;
    bool spans(const Row &row) const;
    static bool listed(const Row &row);
    int firstCell(const Row &row) const;
    int cellLimit(const Row &row, const ItemModel *items) const;
    QList<Row> layout() const;
    QList<SectionState> sectionStates() const;
    void sync();
    void observeItems();

    QPointer<BrowseModel> m_page;
    QList<Row> m_rows;
    QList<SectionState> m_sectionStates;
    QList<QMetaObject::Connection> m_connections;
    QSet<ItemModel *> m_observed;
    int m_columns = 4;
    int m_matches = 0;
};

}
