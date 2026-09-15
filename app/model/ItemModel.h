#pragma once

#include "Item.h"

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QStringList>

namespace model {

class ItemModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Items are supplied by a browse page")

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    explicit ItemModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    const QList<Item> &items() const { return m_items; }
    void append(const QList<Item> &items);
    quint64 identity(int index) const;
    bool move(int from, int to);
    int entryIndex(const QString &setVideoId) const;
    void applyRating(const QString &videoId, int rating);
    void applyPinned(const QString &key, bool pinned);
    void applyInLibrary(const QString &key, bool inLibrary);
    void applyLater(const QString &videoId, bool later);
    void applyPlayed(const QString &videoId, bool played);
    void removeEntries(const QStringList &setVideoIds);
    Q_INVOKABLE model::Item get(int index) const;
    Q_INVOKABLE QList<media::Track> tracks() const;
    Q_INVOKABLE QList<media::Track> tracksAt(const QList<int> &rows) const;
    Q_INVOKABLE int trackIndex(int index) const;
    bool matches(int index, const QString &needle) const;

Q_SIGNALS:
    void countChanged();

private:
    QList<Item> m_items;
    QList<quint64> m_identities;
    quint64 m_nextIdentity = 1;
};

}
