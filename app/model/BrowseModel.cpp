#include "BrowseModel.h"

#include "auth/Account.h"
#include "core/Logging.h"
#include "library/LibraryActions.h"

#include <QPointer>
#include <QJsonArray>

#include <algorithm>
#include <utility>

namespace {

enum Role {
    Title = Qt::UserRole,
    Subtitle,
    Items,
    Horizontal,
    Navigation,
    Ranked,
    HasMore,
    Loading,
    More,
    Card,
    Lead,
    Description,
    Message,
};

}

namespace model {

BrowseModel::BrowseModel(const Item &source, QString endpoint, QObject *parent)
    : QAbstractListModel(parent)
    , m_endpoints(innertube::Session::instance())
    , m_source(source)
    , m_header(source)
    , m_endpoint(std::move(endpoint))
{
    m_searchDelay.setSingleShot(true);
    m_searchDelay.setInterval(250);
    connect(&m_searchDelay, &QTimer::timeout, this, &BrowseModel::reload);

    const auto &actions = library::LibraryActions::instance();
    connect(&actions, &library::LibraryActions::trackRatingChanged, this,
            [this](const QString &videoId, int rating) {
        for (const Section &section : std::as_const(m_sections))
            section.items->applyRating(videoId, rating);
    });
    connect(&actions, &library::LibraryActions::savedChanged, this,
            [this](const QString &playlistId, bool saved) {
        if (m_header.savePlaylistId != playlistId || m_header.saved == saved)
            return;
        m_header.saved = saved;
        Q_EMIT stateChanged();
    });
    connect(&actions, &library::LibraryActions::subscriptionChanged, this,
            [this](const QString &channelId, bool subscribed) {
        if (followTarget() != channelId || m_header.subscribed == subscribed)
            return;
        m_header.subscribed = subscribed;
        Q_EMIT stateChanged();
    });
    connect(&actions, &library::LibraryActions::pinChanged, this,
            [this](const QString &key, bool pinned) {
        for (const Section &section : std::as_const(m_sections))
            section.items->applyPinned(key, pinned);
        if (m_header.key() != key || m_header.pinned == pinned)
            return;
        m_header.pinned = pinned;
        Q_EMIT stateChanged();
    });
    connect(&actions, &library::LibraryActions::libraryChanged, this,
            [this](const QString &key, bool inLibrary) {
        for (const Section &section : std::as_const(m_sections))
            section.items->applyInLibrary(key, inLibrary);
        if (m_header.key() != key || m_header.actions.inLibrary == inLibrary)
            return;
        m_header.actions.inLibrary = inLibrary;
        Q_EMIT stateChanged();
    });
    connect(&actions, &library::LibraryActions::laterChanged, this,
            [this](const QString &videoId, bool later) {
        for (const Section &section : std::as_const(m_sections))
            section.items->applyLater(videoId, later);
        if (m_header.track.videoId != videoId || m_header.actions.later == later)
            return;
        m_header.actions.later = later;
        Q_EMIT stateChanged();
    });
    connect(&actions, &library::LibraryActions::playedChanged, this,
            [this](const QString &videoId, bool played) {
        for (const Section &section : std::as_const(m_sections))
            section.items->applyPlayed(videoId, played);
        if (m_header.track.videoId != videoId || m_header.played == played)
            return;
        m_header.played = played;
        Q_EMIT stateChanged();
    });
    connect(&actions, &library::LibraryActions::playlistItemsRemoved, this,
            [this](const QString &playlistId, const QStringList &setVideoIds) {
        if (!owns(playlistId))
            return;
        for (const Section &section : std::as_const(m_sections))
            section.items->removeEntries(setVideoIds);
        Q_EMIT stateChanged();
    });
    connect(&actions, &library::LibraryActions::playlistReordered, this,
            [this](const QString &playlistId) {
        if (owns(playlistId))
            reload();
    });
    connect(&actions, &library::LibraryActions::playlistEdited, this,
            [this](const QString &playlistId, const QString &name, const QString &description,
                   const QString &privacy) {
        if (!owns(playlistId))
            return;
        if (!name.isEmpty())
            m_header.title = name;
        m_header.description = description;
        m_header.privacy = privacy;
        Q_EMIT stateChanged();
    });
}

bool BrowseModel::detail() const
{
    static const QStringList kinds {QStringLiteral("album"),   QStringLiteral("playlist"),
                                    QStringLiteral("artist"),  QStringLiteral("podcast"),
                                    QStringLiteral("episode"), QStringLiteral("profile")};
    return kinds.contains(kind());
}

bool BrowseModel::sequential() const
{
    return detail() || kind() == QLatin1String("library") || kind() == QLatin1String("history")
        || local();
}

bool BrowseModel::gridSkeleton() const
{
    static const QStringList listed {QStringLiteral("FEmusic_liked_videos"),
                                     QStringLiteral("FEmusic_library_privately_owned_tracks")};
    if (local())
        return false;
    if (kind() == QLatin1String("library"))
        return !listed.contains(m_source.browseId);
    return kind() == QLatin1String("feed") || kind() == QLatin1String("category");
}

bool BrowseModel::followable() const
{
    return (kind() == QLatin1String("artist") || kind() == QLatin1String("profile"))
        && !followTarget().isEmpty() && followTarget() != auth::Account::instance().channelId();
}

int BrowseModel::sortIndex() const
{
    for (int i = 0; i < m_sortOptions.size(); ++i) {
        if (m_sortOptions.at(i).selected)
            return i;
    }
    return -1;
}

int BrowseModel::scopeIndex() const
{
    for (int i = 0; i < m_scopes.size(); ++i) {
        if (m_scopes.at(i).selected)
            return i;
    }
    return m_scopes.isEmpty() ? -1 : 0;
}

bool BrowseModel::filterable() const
{
    return std::ranges::count_if(m_chips, [](const Chip &chip) {
        return !chip.clearing() && !chip.browseId.isEmpty();
    }) > 1;
}

int BrowseModel::filterIndex() const
{
    int chosen = -1;
    for (int i = 0; i < m_chips.size(); ++i) {
        const Chip &chip = m_chips.at(i);
        if (chip.clearing() || chip.browseId.isEmpty())
            continue;
        if (chosen < 0 || chip.selected)
            chosen = i;
    }
    return chosen;
}

bool BrowseModel::chipSelected() const
{
    return std::ranges::any_of(m_chips, [](const Chip &chip) {
        return chip.selected && chip.reloads() && !chip.deselection.isEmpty();
    });
}

bool BrowseModel::messaged() const
{
    return std::ranges::any_of(m_sections, [](const Section &section) {
        return !section.shelf.message.isEmpty() && section.items->rowCount() == 0;
    });
}

bool BrowseModel::owns(const QString &playlistId) const
{
    return !playlistId.isEmpty() && playlistTarget() == playlistId;
}

bool BrowseModel::reorderable() const
{
    if (kind() != QLatin1String("playlist") || !owned() || m_refreshing || !m_filter.isEmpty()
        || !auth::Account::instance().signedIn())
        return false;
    const int selected = sortIndex();
    return selected >= 0 && m_sortOptions.at(selected).videoOrder == 0;
}

bool BrowseModel::movePlaylistItem(const QString &setVideoId, int to)
{
    if (!reorderable() || m_reordering || m_sections.isEmpty())
        return false;
    ItemModel *items = m_sections.first().items;
    const int from = items->entryIndex(setVideoId);
    if (from < 0 || to < 0 || to >= items->rowCount() || from == to)
        return false;
    const QString originalPredecessor =
        from > 0 ? items->get(from - 1).actions.setVideoId : QString();
    const int predecessorIndex = to > from ? to : to - 1;
    const QString predecessor =
        predecessorIndex >= 0 ? items->get(predecessorIndex).actions.setVideoId : QString();
    if ((from > 0 && originalPredecessor.isEmpty()) || (to > 0 && predecessor.isEmpty()))
        return false;

    m_reordering = true;
    items->move(from, to);
    Q_EMIT stateChanged();
    const QPointer<BrowseModel> guard(this);
    const QPointer<ItemModel> target(items);
    library::LibraryActions::instance().movePlaylistItem(
        playlistTarget(), setVideoId, predecessor, [guard, target, setVideoId, from](bool success) {
        if (!guard)
            return;
        if (!success && target) {
            const int current = target->entryIndex(setVideoId);
            if (current >= 0)
                target->move(current, from);
        } else if (!success) {
            guard->reload();
        }
        guard->m_reordering = false;
        Q_EMIT guard->stateChanged();
    });
    return true;
}

void BrowseModel::setFilter(const QString &filter)
{
    if (m_filter == filter)
        return;
    m_filter = filter;
    if (local()) {
        Q_EMIT filterChanged();
        reload();
        return;
    }
    ++m_generation;
    m_refreshing = true;
    m_loading = true;
    Q_EMIT filterChanged();
    Q_EMIT stateChanged();
    m_searchDelay.start();
}

void BrowseModel::applySort(int index)
{
    if (m_loading || m_reordering || index < 0 || index >= m_sortOptions.size())
        return;
    const SortOption option = m_sortOptions.at(index);
    if (option.selected)
        return;
    if (option.stored() && m_header.owned) {
        const QList<SortOption> previous = m_sortOptions;
        for (int i = 0; i < m_sortOptions.size(); ++i)
            m_sortOptions[i].selected = i == index;
        m_refreshing = true;
        m_loading = true;
        Q_EMIT stateChanged();
        const QPointer<BrowseModel> guard(this);
        library::LibraryActions::instance().setPlaylistOrder(playlistTarget(), option,
                                                             [guard, previous](bool success) {
            if (!guard || success)
                return;
            guard->m_sortOptions = previous;
            guard->m_loading = false;
            guard->m_refreshing = false;
            Q_EMIT guard->stateChanged();
        });
        return;
    }
    if (!option.valid())
        return;
    m_pendingSort = index;
    m_source.params = option.params;
    m_reloadContinuation = option.continuation;
    m_formValue = option.formValue;
    reload();
}

void BrowseModel::refine(const QString &params, const QList<Chip> &chips, const QList<Chip> &scopes)
{
    m_source.params = params;
    m_chips = chips;
    m_scopes = scopes;
    m_reloadContinuation.clear();
    m_formValue.clear();
    reload();
}

void BrowseModel::selectChip(int index)
{
    if (m_loading || index < 0 || index >= m_chips.size())
        return;
    const Chip chip = m_chips.at(index);
    if (chip.reloads()) {
        reloadChip(index);
        return;
    }
    if (chip.browseId.isEmpty()
        || (chip.browseId == m_source.browseId && chip.params == m_source.params))
        return;
    for (int i = 0; i < m_chips.size(); ++i)
        m_chips[i].selected = i == index;
    m_source.browseId = chip.browseId;
    m_source.params = chip.params;
    m_reloadContinuation.clear();
    m_formValue.clear();
    reload();
}

void BrowseModel::reloadChip(int index)
{
    const Chip &chip = m_chips.at(index);
    const QString token = chip.selected ? chip.deselection : chip.continuation;
    if (token.isEmpty())
        return;
    const bool selecting = !chip.selected;
    for (int i = 0; i < m_chips.size(); ++i)
        m_chips[i].selected = selecting && i == index;
    m_reloadContinuation = token;
    m_formValue.clear();
    reload();
}

void BrowseModel::clearChip()
{
    if (m_loading)
        return;
    for (int i = 0; i < m_chips.size(); ++i) {
        if (m_chips.at(i).selected && m_chips.at(i).reloads()) {
            reloadChip(i);
            return;
        }
    }
}

QString BrowseModel::saveTarget() const
{
    return m_header.savePlaylistId;
}

QString BrowseModel::followTarget() const
{
    return m_header.channelId;
}

void BrowseModel::toggleSaved()
{
    library::LibraryActions::instance().setSaved(saveTarget(), !m_header.saved);
}

void BrowseModel::toggleSubscribed()
{
    if (!followable())
        return;
    library::LibraryActions::instance().setSubscribed(followTarget(), !m_header.subscribed);
}

int BrowseModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_sections.size();
}

QVariant BrowseModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_sections.size())
        return {};
    const Section &section = m_sections.at(index.row());
    switch (role) {
    case Title: return section.shelf.title;
    case Subtitle: return section.shelf.subtitle;
    case Items: return QVariant::fromValue(section.items);
    case Horizontal: return section.shelf.horizontal;
    case Navigation: return section.shelf.navigation;
    case HasMore: return !section.shelf.continuation.isEmpty();
    case Loading: return section.loading;
    case Ranked: return section.shelf.ranked;
    case More: return QVariant::fromValue(section.shelf.more);
    case Card: return section.shelf.card;
    case Lead: return QVariant::fromValue(section.shelf.lead);
    case Description: return section.shelf.description;
    case Message: return section.shelf.message;
    default: return {};
    }
}

QHash<int, QByteArray> BrowseModel::roleNames() const
{
    return {{Title, "sectionTitle"},
            {Subtitle, "sectionSubtitle"},
            {Items, "entries"},
            {Horizontal, "horizontal"},
            {Navigation, "navigation"},
            {Ranked, "ranked"},
            {HasMore, "moreAvailable"},
            {Loading, "busy"},
            {More, "moreDestination"},
            {Card, "card"},
            {Lead, "lead"},
            {Description, "sectionDescription"},
            {Message, "sectionMessage"}};
}

bool BrowseModel::playable() const
{
    for (const Section &section : m_sections) {
        for (const Item &item : section.items->items()) {
            if (item.playable())
                return true;
        }
    }
    return false;
}

QList<media::Track> BrowseModel::tracks() const
{
    QList<media::Track> result;
    for (const Section &section : m_sections) {
        if (m_source.kind != QLatin1String("feed") && m_source.kind != QLatin1String("search")
            && section.shelf.horizontal)
            continue;
        if (section.shelf.card)
            continue;
        result.append(section.items->tracks());
        if (!result.isEmpty()
            && (m_source.kind == QLatin1String("album")
                || m_source.kind == QLatin1String("playlist")
                || m_source.kind == QLatin1String("podcast")))
            break;
    }
    return result;
}

void BrowseModel::replaceTracks(const QList<media::Track> &tracks)
{
    clearSections();
    innertube::parsers::Page page;
    Shelf shelf;
    for (const media::Track &track : tracks) {
        Item item;
        item.title = track.title;
        item.subtitle = track.artist;
        item.artId = track.artId;
        item.kind = QStringLiteral("song");
        item.track = track;
        shelf.items.append(item);
    }
    if (!shelf.items.isEmpty())
        page.shelves.append(shelf);
    append(std::move(page), -1);
    Q_EMIT stateChanged();
}

void BrowseModel::clearSections()
{
    beginResetModel();
    for (const Section &section : m_sections)
        section.items->deleteLater();
    m_sections.clear();
    endResetModel();
}

void BrowseModel::reload()
{
    m_searchDelay.stop();
    const quint64 generation = ++m_generation;
    m_error.clear();
    m_unreachable = false;
    m_continuation.clear();
    m_consumed.clear();
    m_nextFallback = false;
    m_refreshing = true;
    m_loading = true;
    clearSections();
    Q_EMIT reloadStarted();
    Q_EMIT stateChanged();

    if (!m_filter.isEmpty() && searchable()) {
        fetchSearch(generation);
        return;
    }

    const QPointer<BrowseModel> guard(this);
    const auto handler = [guard, generation](const innertube::Reply &reply) {
        if (guard)
            guard->accept(reply, generation, -1, {});
    };
    if (!m_reloadContinuation.isEmpty()) {
        m_endpoints.continuation(QStringLiteral("browse"), m_reloadContinuation, handler);
    } else if (!m_formValue.isEmpty()) {
        const QJsonObject body {
            {QStringLiteral("browseId"), m_source.browseId},
            {QStringLiteral("formData"),
             QJsonObject {{QStringLiteral("selectedValues"), QJsonArray {m_formValue}}}},
        };
        m_endpoints.action(QStringLiteral("browse"), body, handler);
    } else if (m_endpoint == QLatin1String("search")) {
        m_endpoints.search(m_source.title, m_source.params, handler);
    } else if (m_source.browseId.isEmpty() && !m_source.playlistId.isEmpty()) {
        fetchNext(generation);
    } else {
        m_endpoints.browse(m_source.browseId, m_source.params, handler);
    }
}

void BrowseModel::fetchSearch(quint64 generation)
{
    const QPointer<BrowseModel> guard(this);
    m_endpoints.action(QStringLiteral("get_playlist_filter_search_metadata"),
                       {{QStringLiteral("playlistId"), playlistTarget()}},
                       [guard, generation](const innertube::Reply &reply) {
        if (!guard || generation != guard->m_generation)
            return;
        if (!reply.ok()) {
            guard->accept(reply, generation, -1, {});
            return;
        }
        guard->m_searchMatches = {};
        guard->m_searchOffset = 0;
        const auto tracks = reply.json.value(QStringLiteral("tracks")).toArray();
        for (const QJsonValue &value : tracks) {
            const QJsonObject track = value.toObject();
            QString text = track.value(QStringLiteral("trackName")).toString();
            for (const QJsonValue &artist : track.value(QStringLiteral("artistNames")).toArray())
                text += QLatin1Char(' ') + artist.toString();
            if (!text.contains(guard->m_filter, Qt::CaseInsensitive))
                continue;
            QJsonObject identifier {
                {QStringLiteral("videoId"), track.value(QStringLiteral("videoId"))},
                {QStringLiteral("encryptedSetVideoId"), track.value(QStringLiteral("setVideoId"))},
            };
            if (track.contains(QStringLiteral("originalVideoId")))
                identifier.insert(QStringLiteral("originalVideoId"),
                                  track.value(QStringLiteral("originalVideoId")));
            guard->m_searchMatches.append(identifier);
        }
        guard->fetchSearchRows(generation);
    });
}

void BrowseModel::fetchSearchRows(quint64 generation)
{
    QJsonArray identifiers;
    const int end = qMin(m_searchOffset + 20, int(m_searchMatches.size()));
    for (int i = m_searchOffset; i < end; ++i)
        identifiers.append(m_searchMatches.at(i));
    if (identifiers.isEmpty()) {
        m_loading = false;
        m_refreshing = false;
        Q_EMIT stateChanged();
        return;
    }
    const QJsonObject form {
        {QStringLiteral("playlistId"), playlistTarget()},
        {QStringLiteral("playlistVideoItemIdentifiers"), identifiers},
    };
    const QJsonObject body {
        {QStringLiteral("browseId"), QStringLiteral("FEplaylist_filter_search")},
        {QStringLiteral("formData"),
         QJsonObject {{QStringLiteral("playlistFilterSearchFormData"), form}}},
    };
    const QPointer<BrowseModel> guard(this);
    m_endpoints.action(QStringLiteral("browse"), body,
                       [guard, generation, end](const innertube::Reply &reply) {
        if (!guard || generation != guard->m_generation)
            return;
        guard->accept(reply, generation, -1, {});
        if (reply.ok())
            guard->m_searchOffset = end;
        guard->m_continuation = guard->m_searchOffset < guard->m_searchMatches.size()
            ? QStringLiteral("playlist-search")
            : QString();
        Q_EMIT guard->stateChanged();
    });
}

void BrowseModel::loadMore(int shelf)
{
    if (!m_filter.isEmpty() && searchable()) {
        if (!m_loading && m_searchOffset < m_searchMatches.size()) {
            m_loading = true;
            Q_EMIT stateChanged();
            fetchSearchRows(m_generation);
        }
        return;
    }
    if (shelf < -1 || shelf >= m_sections.size())
        return;
    QString token;
    if (shelf >= 0) {
        Section &section = m_sections[shelf];
        if (section.loading || section.shelf.continuation.isEmpty() || m_loading)
            return;
        section.loading = true;
        token = section.shelf.continuation;
        Q_EMIT dataChanged(index(shelf), index(shelf), {Loading});
    } else {
        if (m_loading || m_continuation.isEmpty())
            return;
        m_loading = true;
        token = m_continuation;
    }
    m_error.clear();
    m_unreachable = false;
    Q_EMIT stateChanged();

    const quint64 generation = m_generation;
    const QPointer<BrowseModel> guard(this);
    m_endpoints.continuation(m_nextFallback ? QStringLiteral("next") : m_endpoint, token,
                             [guard, generation, shelf, token](const innertube::Reply &reply) {
        if (guard)
            guard->accept(reply, generation, shelf, token);
    });
}

void BrowseModel::recover()
{
    if (m_error.isEmpty() || m_loading)
        return;
    if (m_sections.isEmpty()) {
        reload();
        return;
    }
    m_error.clear();
    m_unreachable = false;
    Q_EMIT stateChanged();
}

bool BrowseModel::hasTracks(const innertube::parsers::Page &page) const
{
    for (const Shelf &shelf : page.shelves) {
        for (const Item &item : shelf.items) {
            if (item.playable())
                return true;
        }
    }
    return false;
}

void BrowseModel::fetchNext(quint64 generation)
{
    m_nextFallback = true;
    QString playlistId = m_source.playlistId;
    if (playlistId.isEmpty())
        playlistId = m_source.browseId;
    if (playlistId.startsWith(QLatin1String("VL")))
        playlistId.remove(0, 2);
    const QPointer<BrowseModel> guard(this);
    const QString params = m_source.browseId.isEmpty() ? m_source.params : QString();
    m_endpoints.next(playlistId, {}, params, [guard, generation](const innertube::Reply &reply) {
        if (guard)
            guard->accept(reply, generation, -1, {});
    });
}

void BrowseModel::accept(const innertube::Reply &reply, quint64 generation, int shelf,
                         const QString &token)
{
    if (generation != m_generation)
        return;
    innertube::parsers::Page page;
    if (reply.ok())
        page = innertube::parsers::RendererParser::parse(reply.json);
    if (token.isEmpty() && m_source.kind == QLatin1String("playlist") && m_filter.isEmpty()
        && reply.ok() && !m_nextFallback && !hasTracks(page)) {
        if (!page.header.title.isEmpty()) {
            const QString art = m_header.artId;
            m_header = page.header;
            if (!art.isEmpty())
                m_header.artId = art;
        }
        fetchNext(generation);
        return;
    }
    if (shelf >= 0)
        m_sections[shelf].loading = false;
    else
        m_loading = false;

    if (!reply.ok()) {
        m_error = tr("Could not load content. Check your connection and retry.");
        m_unreachable = reply.unreachable;
        if (m_unreachable)
            qCDebug(logInnerTube) << "browse waits for the network" << m_source.title;
        else
            qCWarning(logInnerTube) << "browse failed" << m_source.title << reply.error;
    } else {
        QSet<QString> &consumed = shelf >= 0 ? m_sections[shelf].consumed : m_consumed;
        if (!token.isEmpty())
            consumed.insert(token);
        if (consumed.contains(page.continuation))
            page.continuation.clear();
        for (Shelf &section : page.shelves) {
            if (consumed.contains(section.continuation))
                section.continuation.clear();
        }
        const bool receivedOptions = !page.sortOptions.isEmpty();
        append(std::move(page), shelf);
        if (m_pendingSort >= 0 && !receivedOptions) {
            for (int i = 0; i < m_sortOptions.size(); ++i)
                m_sortOptions[i].selected = i == m_pendingSort;
        }
        m_pendingSort = -1;
        qCInfo(logInnerTube) << "loaded" << m_header.title << m_sections.size() << "sections";
    }
    m_refreshing = false;
    if (shelf >= 0)
        Q_EMIT dataChanged(index(shelf), index(shelf));
    Q_EMIT stateChanged();
}

void BrowseModel::enrich(QList<Item> &items) const
{
    auto &actions = library::LibraryActions::instance();
    for (Item &item : items) {
        actions.applyOverrides(item);
        if (item.actions.sourcePlaylistId.isEmpty() && m_source.kind == QLatin1String("playlist"))
            item.actions.sourcePlaylistId = playlistTarget();
        if (!item.playable())
            continue;
        if (m_source.browseId == QLatin1String("FEmusic_liked_videos")) {
            item.track.liked = true;
            item.track.ratingKnown = true;
        }
        if (item.track.ratingKnown)
            actions.rememberRating(item.track.videoId,
                                   item.track.liked          ? 1
                                       : item.track.disliked ? -1
                                                             : 0);
        const int rating = actions.rating(item.track.videoId,
                                          item.track.liked          ? 1
                                              : item.track.disliked ? -1
                                                                    : 0);
        item.track.liked = rating > 0;
        item.track.disliked = rating < 0;
        if (item.artId.isEmpty()) {
            item.artId = m_header.artId;
            item.track.artId = item.artId;
        }
        if (m_source.kind == QLatin1String("album")) {
            if (item.track.album.isEmpty())
                item.track.album = m_header.title;
            if (item.track.artist.isEmpty())
                item.track.artist = m_header.track.artist;
        }
        if (m_source.kind == QLatin1String("artist") && item.track.artist.isEmpty())
            item.track.artist = m_header.title;
        if (m_source.kind == QLatin1String("podcast") && item.track.episode) {
            if (item.track.artist.isEmpty())
                item.track.artist = m_header.title;
            if (item.actions.podcastId.isEmpty())
                item.actions.podcastId = m_source.browseId;
        }
    }
}

void BrowseModel::append(innertube::parsers::Page page, int shelf)
{
    if (!page.header.title.isEmpty())
        m_header.title = page.header.title;
    if (!page.header.subtitle.isEmpty())
        m_header.subtitle = page.header.subtitle;
    if (!page.header.description.isEmpty())
        m_header.description = page.header.description;
    if (!page.header.byline.isEmpty()) {
        m_header.byline = page.header.byline;
        m_header.bylineArtId = page.header.bylineArtId;
    }
    if (page.header.track.valid() && !m_header.track.valid())
        m_header.track = page.header.track;
    if (!page.header.progressLabel.isEmpty()) {
        m_header.progress = page.header.progress;
        m_header.progressLabel = page.header.progressLabel;
        m_header.played = page.header.played;
    }
    if (m_header.artId.isEmpty() && !page.header.artId.isEmpty())
        m_header.artId = page.header.artId;
    if (!page.header.savePlaylistId.isEmpty()) {
        m_header.savePlaylistId = page.header.savePlaylistId;
        m_header.saved = page.header.saved;
    }
    if (!page.header.channelId.isEmpty()) {
        m_header.channelId = page.header.channelId;
        m_header.subscribed = page.header.subscribed;
    }
    if (page.header.deletable)
        m_header.deletable = true;
    if (page.header.owned) {
        m_header.owned = true;
        m_header.privacy = page.header.privacy;
        m_header.description = page.header.description;
    }
    if (page.header.actions != ItemActions())
        m_header.actions = page.header.actions;
    if (!page.header.title.isEmpty())
        m_header.pinned = page.header.pinned;
    if (!page.header.track.artist.isEmpty())
        m_header.track.artist = page.header.track.artist;
    if (!page.sortOptions.isEmpty())
        m_sortOptions = page.sortOptions;
    if (shelf < 0 && (!page.chips.isEmpty() || !page.scopes.isEmpty())) {
        m_chips = page.chips;
        m_scopes = page.scopes;
    }

    if (shelf >= 0) {
        Section &target = m_sections[shelf];
        target.shelf.continuation = page.continuation;
        for (Shelf &section : page.shelves) {
            enrich(section.items);
            target.items->append(section.items);
            if (!section.continuation.isEmpty())
                target.shelf.continuation = section.continuation;
        }
        return;
    }
    m_continuation = page.continuation;
    if (page.shelves.isEmpty())
        return;
    const int first = m_sections.size();
    beginInsertRows({}, first, first + page.shelves.size() - 1);
    for (Shelf &section : page.shelves) {
        enrich(section.items);
        auto *items = new ItemModel(this);
        items->append(section.items);
        section.items.clear();
        m_sections.append({std::move(section), items, false, {}});
    }
    endInsertRows();
}

}
