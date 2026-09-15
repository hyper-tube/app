#include "Browser.h"

#include "SearchHistory.h"
#include "auth/Account.h"
#include "core/Localization.h"
#include "innertube/parsers/RendererParser.h"
#include "library/LibraryActions.h"
#include "model/DownloadsModel.h"
#include "model/OfflineSearchModel.h"
#include "net/Connectivity.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QSettings>
#include <QTimer>

#include <algorithm>

namespace {

struct LibrarySurface
{
    const char *label;
    QString browseId;
};

constexpr int kLibrarySettleMs = 1500;
constexpr int kLibrarySection = 2;
constexpr int kDownloadsSection = 3;

const QString kLandingBrowseId = QStringLiteral("FEmusic_library_landing");
const QString kUploadsBrowseId = QStringLiteral("FEmusic_library_privately_owned_tracks");
const QString kPlaylistsBrowseId = QStringLiteral("FEmusic_liked_playlists");
const QString kHistoryBrowseId = QStringLiteral("FEmusic_history");
const QString kLibraryTabsKey = QStringLiteral("library/tabs");

const LibrarySurface kHome {QT_TRANSLATE_NOOP("media::Browser", "Home"),
                            QStringLiteral("FEmusic_home")};
const LibrarySurface kExplore {QT_TRANSLATE_NOOP("media::Browser", "Explore"),
                               QStringLiteral("FEmusic_explore")};
const LibrarySurface kLanding {QT_TRANSLATE_NOOP("media::Browser", "All"), kLandingBrowseId};

const QList<LibrarySurface> kServerSurfaces {
    {QT_TRANSLATE_NOOP("media::Browser", "Playlists"), kPlaylistsBrowseId},
    {QT_TRANSLATE_NOOP("media::Browser", "Albums"), QStringLiteral("FEmusic_liked_albums")},
    {QT_TRANSLATE_NOOP("media::Browser", "Artists"),
     QStringLiteral("FEmusic_library_corpus_track_artists")},
    {QT_TRANSLATE_NOOP("media::Browser", "Songs"), QStringLiteral("FEmusic_liked_videos")},
};

const LibrarySurface kUploads {QT_TRANSLATE_NOOP("media::Browser", "Uploads"), kUploadsBrowseId};
const LibrarySurface kHistory {QT_TRANSLATE_NOOP("media::Browser", "History"), kHistoryBrowseId};

const QHash<QString, const char *> kPlaceholders {
    {QStringLiteral("artist"), QT_TRANSLATE_NOOP("media::Browser", "Artist")},
    {QStringLiteral("album"), QT_TRANSLATE_NOOP("media::Browser", "Album")},
    {QStringLiteral("podcast"), QT_TRANSLATE_NOOP("media::Browser", "Podcast")},
    {QStringLiteral("episode"), QT_TRANSLATE_NOOP("media::Browser", "Episode")},
    {QStringLiteral("profile"), QT_TRANSLATE_NOOP("media::Browser", "Profile")},
};

model::Chip tabOf(const LibrarySurface &surface)
{
    model::Chip chip;
    chip.title = media::Browser::tr(surface.label);
    chip.browseId = surface.browseId;
    return chip;
}

QList<model::Chip> storedTabs()
{
    QList<model::Chip> tabs;
    const QJsonArray stored =
        QJsonDocument::fromJson(QSettings().value(kLibraryTabsKey).toString().toUtf8()).array();
    for (const QJsonValue &value : stored) {
        const QJsonObject entry = value.toObject();
        model::Chip chip;
        chip.title = entry.value(QStringLiteral("title")).toString();
        chip.browseId = entry.value(QStringLiteral("browseId")).toString();
        chip.params = entry.value(QStringLiteral("params")).toString();
        if (!chip.title.isEmpty() && !chip.browseId.isEmpty())
            tabs.append(chip);
    }
    return tabs;
}

void storeTabs(const QList<model::Chip> &tabs)
{
    QJsonArray stored;
    for (const model::Chip &chip : tabs) {
        stored.append(QJsonObject {{QStringLiteral("title"), chip.title},
                                   {QStringLiteral("browseId"), chip.browseId},
                                   {QStringLiteral("params"), chip.params}});
    }
    QSettings().setValue(kLibraryTabsKey,
                         QString::fromUtf8(QJsonDocument(stored).toJson(QJsonDocument::Compact)));
}

model::Item feed(const QString &title, const QString &id)
{
    model::Item item;
    item.title = title;
    item.browseId = id;
    item.kind = QStringLiteral("feed");
    return item;
}

model::BrowseModel *makeFeed(const LibrarySurface &surface, QObject *parent)
{
    return new model::BrowseModel(feed(media::Browser::tr(surface.label), surface.browseId),
                                  QStringLiteral("browse"), parent);
}

void selectMatching(QList<model::Chip> &chips, const QString &params)
{
    for (model::Chip &chip : chips)
        chip.selected = !params.isEmpty() && chip.params == params;
}

}

namespace media {

Browser::Browser(QObject *parent)
    : QObject(parent)
    , m_home(makeFeed(kHome, this))
    , m_explore(makeFeed(kExplore, this))
    , m_downloads(new model::DownloadsModel(this))
    , m_endpoints(innertube::Session::instance())
{
    m_serverTabs = storedTabs();
    m_history.append(m_home);
    QTimer::singleShot(0, m_home, &model::BrowseModel::reload);

    m_signedIn = auth::Account::instance().signedIn();
    connect(&auth::Account::instance(), &auth::Account::changed, this, [this] {
        const bool signedIn = auth::Account::instance().signedIn();
        if (m_signedIn == signedIn)
            return;
        m_signedIn = signedIn;
        discardLibrary();
        m_stale.insert(m_home);
        m_stale.insert(m_explore);
        showSection(m_section);
    });
    connect(&core::Localization::instance(), &core::Localization::resolvedChanged, this,
            &Browser::retranslate);
    connect(&net::Connectivity::instance(), &net::Connectivity::onlineChanged, this,
            &Browser::followConnectivity);
    connect(&net::Connectivity::instance(), &net::Connectivity::checkingChanged, this,
            &Browser::recover);
    connect(&library::LibraryActions::instance(), &library::LibraryActions::collectionChanged, this,
            &Browser::staleLibrary);
    connect(&library::LibraryActions::instance(), &library::LibraryActions::playlistDeleted, this,
            [this](const QString &playlistId) {
        if (!page())
            return;
        const auto &source = page()->source();
        if (source.playlistId != playlistId && source.browseId != playlistId
            && source.browseId != QStringLiteral("VL") + playlistId)
            return;
        if (canGoBack()) {
            back();
            if (page() && page()->kind() != QLatin1String("library"))
                page()->reload();
        } else {
            showSection(kLibrarySection);
        }
    });
}

QList<model::Chip> Browser::libraryTabs() const
{
    QList<model::Chip> tabs {tabOf(kLanding)};
    if (m_serverTabs.isEmpty()) {
        for (const LibrarySurface &surface : kServerSurfaces)
            tabs.append(tabOf(surface));
    } else {
        tabs.append(m_serverTabs);
    }
    tabs.append(tabOf(kUploads));
    return tabs;
}

bool Browser::libraryOwns(const model::BrowseModel *surface) const
{
    return std::ranges::any_of(
        m_library, [surface](const model::BrowseModel *entry) { return entry == surface; });
}

bool Browser::retains(const model::BrowseModel *surface) const
{
    return surface == m_home || surface == m_explore || surface == m_downloads
        || libraryOwns(surface);
}

QString Browser::libraryTabId(int tab) const
{
    return libraryTabs().value(tab).browseId;
}

bool Browser::uploadsTab() const
{
    return libraryTabId(m_libraryTab) == kUploadsBrowseId;
}

bool Browser::playlistsTab() const
{
    return libraryTabId(m_libraryTab) == kPlaylistsBrowseId;
}

model::BrowseModel *Browser::library(int tab)
{
    const model::Chip chip = libraryTabs().value(tab);
    if (const auto found = m_library.constFind(chip.browseId); found != m_library.constEnd())
        return *found;
    model::Item source = feed(chip.title, chip.browseId);
    source.params = chip.params;
    source.kind = QStringLiteral("library");
    auto *surface = new model::BrowseModel(source, QStringLiteral("browse"), this);
    if (chip.browseId == kLandingBrowseId)
        connect(surface, &model::BrowseModel::stateChanged, this, &Browser::adoptLibraryTabs);
    m_library.insert(chip.browseId, surface);
    return surface;
}

void Browser::adoptLibraryTabs()
{
    const model::BrowseModel *landing = m_library.value(kLandingBrowseId);
    if (!landing || landing->loading())
        return;
    QList<model::Chip> tabs;
    for (model::Chip chip : landing->chips()) {
        if (chip.clearing() || chip.browseId.isEmpty() || chip.browseId == kLandingBrowseId)
            continue;
        chip.selected = false;
        tabs.append(chip);
    }
    if (tabs.isEmpty() || tabs == m_serverTabs)
        return;
    const QString current = libraryTabId(m_libraryTab);
    m_serverTabs = tabs;
    storeTabs(tabs);
    const QList<model::Chip> updated = libraryTabs();
    int index = 0;
    for (int tab = 0; tab < updated.size(); ++tab) {
        if (updated.at(tab).browseId == current)
            index = tab;
    }
    Q_EMIT libraryTabsChanged();
    if (m_libraryTab == index)
        return;
    m_libraryTab = index;
    Q_EMIT libraryTabChanged();
}

void Browser::discardLibrary()
{
    for (model::BrowseModel *entry : std::as_const(m_library)) {
        m_history.removeAll(entry);
        m_stale.remove(entry);
        entry->deleteLater();
    }
    m_library.clear();
}

void Browser::staleLibrary()
{
    for (model::BrowseModel *entry : std::as_const(m_library))
        m_stale.insert(entry);
    if (m_section != kLibrarySection || m_history.isEmpty()
        || m_history.last() != m_library.value(libraryTabId(m_libraryTab)))
        return;
    m_stale.remove(m_history.last());
    QTimer::singleShot(kLibrarySettleMs, m_history.last(), &model::BrowseModel::reload);
}

void Browser::retranslate()
{
    clearHistory();
    discardLibrary();
    m_stale.clear();
    m_home->deleteLater();
    m_explore->deleteLater();
    m_home = makeFeed(kHome, this);
    m_explore = makeFeed(kExplore, this);
    Q_EMIT libraryTabsChanged();
    showSection(m_section);
}

void Browser::refresh()
{
    model::BrowseModel *target = page();
    if (!target || target->loading())
        return;
    if (m_section == kLibrarySection && !auth::Account::instance().signedIn())
        return;
    if (m_stale.remove(target) || target->rowCount() == 0)
        target->reload();
}

void Browser::showLibrary()
{
    m_history.append(library(m_libraryTab));
}

void Browser::showLibraryTab(int tab)
{
    if (tab < 0 || tab >= libraryTabs().size()
        || (tab == m_libraryTab && m_section == kLibrarySection))
        return;
    if (m_libraryTab != tab) {
        m_libraryTab = tab;
        Q_EMIT libraryTabChanged();
    }
    showSection(kLibrarySection);
}

QString Browser::searchQuery() const
{
    return page() && page()->kind() == QLatin1String("search") ? page()->source().title : QString();
}

void Browser::clearHistory()
{
    m_autoplay.clear();
    for (model::BrowseModel *entry : std::as_const(m_history)) {
        if (!retains(entry))
            entry->deleteLater();
    }
    m_history.clear();
}

void Browser::showSection(int section)
{
    if (section < 0 || section > kDownloadsSection)
        return;
    m_navigated = true;
    clearHistory();
    m_section = section;
    if (section == kLibrarySection)
        showLibrary();
    else if (section == kDownloadsSection)
        m_history.append(m_downloads);
    else
        m_history.append(section == 0 ? m_home : m_explore);
    Q_EMIT pageChanged();
    refresh();
}

void Browser::search(const QString &query, const QString &params)
{
    const QString trimmed = query.trimmed();
    if (trimmed.isEmpty())
        return;
    model::Item source;
    source.title = trimmed;
    source.kind = QStringLiteral("search");
    source.params = params;
    SearchHistory::instance().remember(trimmed);
    model::BrowseModel *current = page();
    const bool searching = current && current->kind() == QLatin1String("search");
    const bool online = net::Connectivity::instance().online();
    if (searching && !current->local() && online && current->source().title == trimmed) {
        QList<model::Chip> chips = current->chips();
        QList<model::Chip> scopes = current->scopes();
        const bool rescoped = std::ranges::any_of(scopes, [&params](const model::Chip &scope) {
            return !scope.selected && scope.params == params;
        });
        if (rescoped) {
            chips.clear();
            selectMatching(scopes, params);
        } else {
            selectMatching(chips, params);
        }
        current->refine(params, chips, scopes);
        return;
    }
    if (searching)
        m_history.takeLast()->deleteLater();
    if (m_section >= kLibrarySection)
        m_section = 0;
    if (!online) {
        push(new model::OfflineSearchModel(trimmed, this));
        return;
    }
    push(new model::BrowseModel(source, QStringLiteral("search"), this));
}

void Browser::chooseChip(int index)
{
    model::BrowseModel *target = page();
    if (!target || index < 0 || index >= target->chips().size())
        return;
    if (target->kind() != QLatin1String("search")) {
        target->selectChip(index);
        return;
    }
    const QList<model::Chip> &chips = target->chips();
    if (!chips.at(index).selected) {
        search(target->source().title, chips.at(index).params);
        return;
    }
    const auto clearing = std::ranges::find_if(chips, &model::Chip::clearing);
    if (clearing != chips.end())
        search(target->source().title, clearing->params);
}

void Browser::chooseScope(int index)
{
    const model::BrowseModel *target = page();
    if (!target || index < 0 || index >= target->scopes().size()
        || target->scopes().at(index).selected)
        return;
    search(target->source().title, target->scopes().at(index).params);
}

void Browser::retry() const
{
    model::BrowseModel *target = page();
    if (!target)
        return;
    if (target->unreachable() && !net::Connectivity::instance().settled()) {
        net::Connectivity::instance().check();
        return;
    }
    target->recover();
}

void Browser::reloadPage() const
{
    model::BrowseModel *target = page();
    if (!target || target->loading())
        return;
    if (!target->local() && !net::Connectivity::instance().online()) {
        net::Connectivity::instance().check();
        return;
    }
    target->reload();
}

void Browser::showDownloads()
{
    showSection(kDownloadsSection);
}

void Browser::showHistory()
{
    if (page() && page()->kind() == QLatin1String("history"))
        return;
    model::Item source = feed(tr(kHistory.label), kHistory.browseId);
    source.kind = QStringLiteral("history");
    push(new model::BrowseModel(source, QStringLiteral("browse"), this));
}

void Browser::followConnectivity()
{
    if (net::Connectivity::instance().online()) {
        recover();
        return;
    }
    if (!m_navigated && page() == m_home && m_home->rowCount() == 0)
        showDownloads();
}

void Browser::recover()
{
    model::BrowseModel *target = page();
    if (!target || !net::Connectivity::instance().settled())
        return;
    if (target->local() && target->kind() == QLatin1String("search"))
        search(target->title());
    else if (target->unreachable())
        target->recover();
}

void Browser::open(const model::Item &item)
{
    m_autoplay.clear();
    if (!item.valid())
        return;
    if (item.browseId == kLandingBrowseId) {
        showLibraryTab(0);
        return;
    }
    if (item.browseId == kHistoryBrowseId) {
        showHistory();
        return;
    }
    if (item.kind == QLatin1String("search")) {
        search(item.title, item.params);
        return;
    }
    if (item.kind == QLatin1String("library")) {
        const QList<model::Chip> tabs = libraryTabs();
        for (int tab = 0; tab < tabs.size(); ++tab) {
            if (tabs.at(tab).browseId == item.browseId) {
                showLibraryTab(tab);
                return;
            }
        }
    }
    if (item.browseId.isEmpty() && item.playable()) {
        Q_EMIT playRequested({item.track}, 0, {});
        return;
    }
    if (item.kind == QLatin1String("mix")) {
        model::Item mix = item;
        mix.kind = QStringLiteral("playlist");
        push(new model::BrowseModel(mix, QStringLiteral("browse"), this));
        return;
    }
    push(new model::BrowseModel(item, QStringLiteral("browse"), this));
}

void Browser::activate(model::ItemModel *items, int index)
{
    if (!items)
        return;
    const model::Item item = items->get(index);
    if (item.playable() && item.browseId.isEmpty())
        playTrack(items, index);
    else
        open(item);
}

void Browser::playItem(model::ItemModel *items, int index)
{
    if (!items)
        return;
    const model::Item item = items->get(index);
    if (item.playable()) {
        playTrack(items, index);
    } else if (item.valid()) {
        open(item);
        m_autoplay = page();
    }
}

void Browser::playTrack(model::ItemModel *items, int index)
{
    const model::BrowseModel *surface = page();
    if (surface && surface->sequential())
        Q_EMIT playRequested(items->tracks(), items->trackIndex(index), sourceOf(*surface));
    else
        Q_EMIT playRequested({items->get(index).track}, 0, {});
}

void Browser::fetchTracks(const QString &playlistId, const QString &videoId,
                          const std::function<void(const QList<Track> &)> &handler)
{
    const QPointer<Browser> guard(this);
    m_endpoints.next(playlistId, videoId, {}, [guard, handler](const innertube::Reply &reply) {
        if (!guard || !reply.ok())
            return;
        const auto page = innertube::parsers::RendererParser::parse(reply.json);
        QList<Track> tracks;
        for (const model::Shelf &shelf : page.shelves) {
            for (const model::Item &item : shelf.items) {
                if (item.playable())
                    tracks.append(item.track);
            }
        }
        if (!tracks.isEmpty())
            handler(tracks);
    });
}

void Browser::playMix(const model::Item &item)
{
    if (!item.actions.mixable())
        return;
    model::Item source;
    source.title = item.title;
    source.subtitle = item.playable() ? item.track.artist : item.subtitle;
    source.artId = item.artId;
    source.kind = QStringLiteral("mix");
    source.playlistId = item.actions.mixPlaylistId;
    fetchTracks(
        item.actions.mixPlaylistId, item.actions.mixVideoId,
        [this, source](const QList<Track> &tracks) { Q_EMIT playRequested(tracks, 0, source); });
}

void Browser::playEntry(const model::Item &item)
{
    if (item.playable()) {
        Q_EMIT playRequested({item.track}, 0, {});
        return;
    }
    const QString target = queueTarget(item);
    if (target.isEmpty())
        return;
    fetchTracks(target, {}, [this, item](const QList<Track> &tracks) {
        Q_EMIT playRequested(tracks, 0, item);
    });
}

model::Item Browser::sourceOf(const model::BrowseModel &surface)
{
    model::Item source = surface.source();
    if (source.kind == QLatin1String("search") || source.kind == QLatin1String("feed"))
        return {};
    const model::Item &header = surface.entry();
    if (!header.title.isEmpty())
        source.title = header.title;
    if (!header.subtitle.isEmpty())
        source.subtitle = header.subtitle;
    if (!header.artId.isEmpty())
        source.artId = header.artId;
    source.track = {};
    source.actions = {};
    return source;
}

QString Browser::queueTarget(const model::Item &item)
{
    if (!item.actions.shufflePlaylistId.isEmpty())
        return item.actions.shufflePlaylistId;
    if (item.kind == QLatin1String("album") && !item.savePlaylistId.isEmpty())
        return item.savePlaylistId;
    return item.playlistTarget();
}

void Browser::playShuffled(const model::Item &item)
{
    const QString target = queueTarget(item);
    if (target.isEmpty())
        return;
    fetchTracks(target, {}, [this, item](const QList<Track> &tracks) {
        QList<Track> shuffled = tracks;
        std::shuffle(shuffled.begin(), shuffled.end(), *QRandomGenerator::global());
        Q_EMIT playRequested(shuffled, 0, item);
    });
}

void Browser::resolveTracks(const model::Item &item, const QString &intent)
{
    if (item.playable()) {
        Q_EMIT tracksResolved({item.track}, intent);
        return;
    }
    if (intent == QLatin1String("download")) {
        auto *collection = new model::BrowseModel(item, QStringLiteral("browse"), this);
        connect(collection, &model::BrowseModel::stateChanged, collection,
                [this, collection, intent] {
            if (collection->loading())
                return;
            if (!collection->error().isEmpty()) {
                collection->disconnect(collection);
                Q_EMIT resolutionFailed(collection->error());
                collection->deleteLater();
                return;
            }
            if (collection->hasMore()) {
                collection->loadMore();
                return;
            }
            const auto roles = collection->roleNames();
            for (int row = 0; row < collection->rowCount(); ++row) {
                if (collection->data(collection->index(row), roles.key("busy")).toBool())
                    return;
                if (collection->data(collection->index(row), roles.key("moreAvailable")).toBool()) {
                    collection->loadMore(row);
                    return;
                }
            }
            collection->disconnect(collection);
            Q_EMIT tracksResolved(collection->tracks(), intent);
            collection->deleteLater();
        });
        collection->reload();
        return;
    }
    const QString target = queueTarget(item);
    if (target.isEmpty())
        return;
    fetchTracks(target, {}, [this, intent](const QList<Track> &tracks) {
        Q_EMIT tracksResolved(tracks, intent);
    });
}

void Browser::openPage(const QString &browseId, const QString &kind)
{
    if (browseId.isEmpty())
        return;
    model::Item destination;
    destination.title = tr(kPlaceholders.value(kind, QT_TRANSLATE_NOOP("media::Browser", "Page")));
    destination.browseId = browseId;
    destination.kind = kind;
    open(destination);
}

void Browser::playPage()
{
    if (!page())
        return;
    const QList<Track> tracks = page()->tracks();
    if (!tracks.isEmpty())
        Q_EMIT playRequested(tracks, 0, sourceOf(*page()));
}

void Browser::push(model::BrowseModel *target)
{
    m_navigated = true;
    m_history.append(target);
    connect(target, &model::BrowseModel::stateChanged, this, [this, target] {
        if (m_autoplay != target || target->loading())
            return;
        m_autoplay.clear();
        if (page() == target && target->playable())
            playPage();
    });
    Q_EMIT pageChanged();
    target->reload();
}

void Browser::back()
{
    if (!canGoBack())
        return;
    m_navigated = true;
    model::BrowseModel *previous = m_history.takeLast();
    if (!retains(previous))
        previous->deleteLater();
    Q_EMIT pageChanged();
}

}
