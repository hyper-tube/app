#pragma once

#include "Chip.h"
#include "ItemModel.h"
#include "Shelf.h"
#include "SortOption.h"
#include "innertube/Endpoints.h"
#include "innertube/parsers/RendererParser.h"

#include <QAbstractListModel>
#include <QJsonArray>
#include <QSet>
#include <QTimer>

namespace model {

class BrowseModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Pages are supplied by Browser")

    Q_PROPERTY(bool local READ local CONSTANT)
    Q_PROPERTY(bool detail READ detail CONSTANT)
    Q_PROPERTY(QString skeleton READ skeleton NOTIFY stateChanged)
    Q_PROPERTY(bool charts READ charts CONSTANT)
    Q_PROPERTY(int sortIndex READ sortIndex NOTIFY stateChanged)
    Q_PROPERTY(QString title READ title NOTIFY stateChanged)
    Q_PROPERTY(QString subtitle READ subtitle NOTIFY stateChanged)
    Q_PROPERTY(QString description READ description NOTIFY stateChanged)
    Q_PROPERTY(QString privacy READ privacy NOTIFY stateChanged)
    Q_PROPERTY(QString artId READ artId NOTIFY stateChanged)
    Q_PROPERTY(QString kind READ kind CONSTANT)
    Q_PROPERTY(model::Item entry READ entry NOTIFY stateChanged)
    Q_PROPERTY(bool refreshing READ refreshing NOTIFY stateChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY stateChanged)
    Q_PROPERTY(QString error READ error NOTIFY stateChanged)
    Q_PROPERTY(bool unreachable READ unreachable NOTIFY stateChanged)
    Q_PROPERTY(bool hasMore READ hasMore NOTIFY stateChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY stateChanged)
    Q_PROPERTY(bool playable READ playable NOTIFY stateChanged)
    Q_PROPERTY(bool collectable READ collectable NOTIFY stateChanged)
    Q_PROPERTY(bool saved READ saved NOTIFY stateChanged)
    Q_PROPERTY(bool followable READ followable NOTIFY stateChanged)
    Q_PROPERTY(bool subscribed READ subscribed NOTIFY stateChanged)
    Q_PROPERTY(bool deletable READ deletable NOTIFY stateChanged)
    Q_PROPERTY(bool owned READ owned NOTIFY stateChanged)
    Q_PROPERTY(bool reorderable READ reorderable NOTIFY stateChanged)
    Q_PROPERTY(bool reordering READ reordering NOTIFY stateChanged)
    Q_PROPERTY(bool sortable READ sortable NOTIFY stateChanged)
    Q_PROPERTY(bool searchable READ searchable NOTIFY stateChanged)
    Q_PROPERTY(QList<model::SortOption> sortOptions READ sortOptions NOTIFY stateChanged)
    Q_PROPERTY(QList<model::Chip> chips READ chips NOTIFY stateChanged)
    Q_PROPERTY(QList<model::Chip> scopes READ scopes NOTIFY stateChanged)
    Q_PROPERTY(int scopeIndex READ scopeIndex NOTIFY stateChanged)
    Q_PROPERTY(bool filterable READ filterable NOTIFY stateChanged)
    Q_PROPERTY(int filterIndex READ filterIndex NOTIFY stateChanged)
    Q_PROPERTY(bool chipSelected READ chipSelected NOTIFY stateChanged)
    Q_PROPERTY(bool messaged READ messaged NOTIFY stateChanged)
    Q_PROPERTY(QString byline READ byline NOTIFY stateChanged)
    Q_PROPERTY(QString bylineArtId READ bylineArtId NOTIFY stateChanged)
    Q_PROPERTY(QString playlistTarget READ playlistTarget CONSTANT)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)

public:
    BrowseModel(const Item &source, QString endpoint, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    virtual bool local() const { return false; }
    bool detail() const;
    bool sequential() const;
    QString skeleton() const;
    bool charts() const { return m_source.browseId == QLatin1String("FEmusic_charts"); }
    int sortIndex() const;
    int scopeIndex() const;
    bool filterable() const;
    int filterIndex() const;
    bool chipSelected() const;
    bool messaged() const;
    QString title() const { return m_header.title; }
    QString subtitle() const { return m_header.subtitle; }
    QString description() const { return m_header.description; }
    QString privacy() const { return m_header.privacy; }
    QString artId() const { return m_header.artId; }
    QString kind() const { return m_source.kind; }
    const Item &entry() const { return m_header; }
    bool refreshing() const { return m_refreshing; }
    bool loading() const { return m_loading; }
    QString error() const { return m_error; }
    bool unreachable() const { return m_unreachable; }
    bool hasMore() const { return !m_continuation.isEmpty(); }
    bool playable() const;
    bool collectable() const { return !saveTarget().isEmpty(); }
    bool saved() const { return m_header.saved; }
    bool followable() const;
    bool subscribed() const { return m_header.subscribed; }
    bool deletable() const { return m_header.deletable && !playlistTarget().isEmpty(); }
    bool owned() const { return m_header.owned; }
    bool reorderable() const;
    bool reordering() const { return m_reordering; }
    bool sortable() const { return !m_sortOptions.isEmpty(); }
    virtual bool searchable() const
    {
        return kind() == QLatin1String("playlist") || kind() == QLatin1String("podcast");
    }
    const QList<SortOption> &sortOptions() const { return m_sortOptions; }
    const QList<Chip> &chips() const { return m_chips; }
    const QList<Chip> &scopes() const { return m_scopes; }
    QString byline() const { return m_header.byline; }
    QString bylineArtId() const { return m_header.bylineArtId; }
    void refine(const QString &params, const QList<Chip> &chips, const QList<Chip> &scopes);
    QString playlistTarget() const { return m_source.playlistTarget(); }
    const QString &filter() const { return m_filter; }
    void setFilter(const QString &filter);
    QList<media::Track> tracks() const;
    const Item &source() const { return m_source; }

    Q_INVOKABLE virtual void reload();
    Q_INVOKABLE void loadMore(int shelf = -1);
    Q_INVOKABLE void recover();
    Q_INVOKABLE void toggleSaved();
    Q_INVOKABLE void toggleSubscribed();
    Q_INVOKABLE void applySort(int index);
    Q_INVOKABLE void selectChip(int index);
    Q_INVOKABLE void clearChip();
    Q_INVOKABLE bool movePlaylistItem(const QString &setVideoId, int to);

Q_SIGNALS:
    void stateChanged();
    void reloadStarted();
    void filterChanged();

protected:
    void replaceTracks(const QList<media::Track> &tracks);

private:
    struct Section
    {
        Shelf shelf;
        ItemModel *items = nullptr;
        bool loading = false;
        QSet<QString> consumed;
    };

    void clearSections();
    void reloadChip(int index);
    void fetchSearch(quint64 generation);
    void fetchSearchRows(quint64 generation);
    QString saveTarget() const;
    QString followTarget() const;
    bool owns(const QString &playlistId) const;
    void accept(const innertube::Reply &reply, quint64 generation, int shelf, const QString &token);
    void append(innertube::parsers::Page page, int shelf);
    void enrich(QList<Item> &items) const;
    void fetchNext(quint64 generation);
    bool hasTracks(const innertube::parsers::Page &page) const;

    innertube::Endpoints m_endpoints;
    Item m_source;
    Item m_header;
    QString m_endpoint;
    QString m_continuation;
    QString m_error;
    QString m_filter;
    QString m_reloadContinuation;
    QString m_formValue;
    int m_pendingSort = -1;
    QList<Section> m_sections;
    QList<SortOption> m_sortOptions;
    QList<Chip> m_chips;
    QList<Chip> m_scopes;
    QSet<QString> m_consumed;
    quint64 m_generation = 0;
    QTimer m_searchDelay;
    QJsonArray m_searchMatches;
    int m_searchOffset = 0;
    bool m_refreshing = false;
    bool m_loading = false;
    bool m_nextFallback = false;
    bool m_reordering = false;
    bool m_unreachable = false;
};

}
