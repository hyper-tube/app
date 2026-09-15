#include "RendererParser.h"

#include "RendererReader.h"
#include "core/Json.h"
#include "core/Logging.h"

#include <QJsonArray>
#include <QSet>
#include <QUrl>

#include <algorithm>
#include <utility>

namespace {

using innertube::parsers::findFirst;
using innertube::parsers::readContinuation;
using innertube::parsers::readItem;
using innertube::parsers::readText;

const QSet<QString> kItems {
    QStringLiteral("musicResponsiveListItemRenderer"), QStringLiteral("musicTwoRowItemRenderer"),
    QStringLiteral("musicMultiRowListItemRenderer"),   QStringLiteral("playlistPanelVideoRenderer"),
    QStringLiteral("musicNavigationButtonRenderer"),
};

const QSet<QString> kShelves {
    QStringLiteral("musicCarouselShelfRenderer"),
    QStringLiteral("musicShelfRenderer"),
    QStringLiteral("musicPlaylistShelfRenderer"),
    QStringLiteral("musicShelfContinuation"),
    QStringLiteral("musicPlaylistShelfContinuation"),
    QStringLiteral("gridRenderer"),
    QStringLiteral("gridContinuation"),
    QStringLiteral("playlistPanelRenderer"),
    QStringLiteral("playlistPanelContinuation"),
};

const QHash<QString, QString> kScopeGlyphs {
    {QStringLiteral("music_search_catalog"), QStringLiteral("travel_explore")},
    {QStringLiteral("music_search_library"), QStringLiteral("library_music")},
    {QStringLiteral("music_search_upload"), QStringLiteral("cloud_upload")},
};

const QSet<QString> kWrappers {
    QStringLiteral("singleColumnBrowseResultsRenderer"),
    QStringLiteral("twoColumnBrowseResultsRenderer"),
    QStringLiteral("tabbedSearchResultsRenderer"),
    QStringLiteral("tabRenderer"),
    QStringLiteral("sectionListRenderer"),
    QStringLiteral("sectionListContinuation"),
    QStringLiteral("itemSectionRenderer"),
    QStringLiteral("musicQueueRenderer"),
    QStringLiteral("singleColumnMusicWatchNextResultsRenderer"),
    QStringLiteral("tabbedRenderer"),
    QStringLiteral("watchNextTabbedResultsRenderer"),
    QStringLiteral("playlistPanelVideoWrapperRenderer"),
};

void collectItems(const QJsonValue &node, QList<model::Item> &items)
{
    if (node.isArray()) {
        for (const QJsonValue &value : node.toArray())
            collectItems(value, items);
        return;
    }
    const QJsonObject object = node.toObject();
    for (auto it = object.begin(); it != object.end(); ++it) {
        if (kItems.contains(it.key())) {
            const model::Item item = readItem(it.value().toObject());
            if (item.valid())
                items.append(item);
            else
                qCDebug(logInnerTube) << "skipping unplayable item" << it.key();
            return;
        }
        if (it.key() == QLatin1String("primaryRenderer") || kWrappers.contains(it.key())
            || it.key() == QLatin1String("contents") || it.key() == QLatin1String("items"))
            collectItems(it.value(), items);
    }
}

QJsonObject withoutSortMenu(const QJsonObject &header)
{
    QJsonObject stripped = header;
    stripped.remove(QStringLiteral("startItems"));
    const QJsonObject aligned =
        header.value(QStringLiteral("musicSideAlignedItemRenderer")).toObject();
    if (!aligned.isEmpty())
        stripped.insert(QStringLiteral("musicSideAlignedItemRenderer"), withoutSortMenu(aligned));
    return stripped;
}

QString reloadToken(const QJsonValue &command)
{
    return QUrl::fromPercentEncoding(findFirst(command, QStringLiteral("reloadContinuationData"))
                                         .toObject()
                                         .value(QStringLiteral("continuation"))
                                         .toString()
                                         .toUtf8());
}

QList<model::SortOption> readBrowseOptions(const QJsonValue &node)
{
    QHash<QString, QJsonObject> choices;
    QJsonValue mutations =
        core::json::at(node,
                       {QStringLiteral("frameworkUpdates"), QStringLiteral("entityBatchUpdate"),
                        QStringLiteral("mutations")});
    if (!mutations.isArray())
        mutations = findFirst(node, QStringLiteral("mutations"));
    for (const QJsonValue &value : mutations.toArray()) {
        const QJsonObject choice =
            findFirst(value, QStringLiteral("musicFormBooleanChoice")).toObject();
        if (!choice.isEmpty())
            choices.insert(choice.value(QStringLiteral("id")).toString(), choice);
    }

    QList<model::SortOption> options;
    const QJsonObject menu =
        findFirst(node, QStringLiteral("musicMultiSelectMenuRenderer")).toObject();
    for (const QJsonValue &value : menu.value(QStringLiteral("options")).toArray()) {
        const QJsonObject entry =
            value.toObject().value(QStringLiteral("musicMultiSelectMenuItemRenderer")).toObject();
        const QJsonObject choice =
            choices.value(entry.value(QStringLiteral("formItemEntityKey")).toString());
        model::SortOption option;
        option.title = readText(entry.value(QStringLiteral("title")));
        option.selected = choice.value(QStringLiteral("selected")).toBool();
        option.continuation = reloadToken(entry.value(QStringLiteral("selectedCommand")));
        option.formValue = choice.value(QStringLiteral("opaqueToken")).toString();
        if (!option.title.isEmpty() && option.valid())
            options.append(option);
    }
    return options;
}

QList<model::SortOption> readSortOptions(const QJsonValue &node)
{
    QList<model::SortOption> options;
    const QJsonObject menu =
        findFirst(node, QStringLiteral("sortFilterSubMenuRenderer")).toObject();
    for (const QJsonValue &value : menu.value(QStringLiteral("subMenuItems")).toArray()) {
        const QJsonObject entry = value.toObject();
        model::SortOption option;
        option.title = readText(entry.value(QStringLiteral("title")));
        option.selected = entry.value(QStringLiteral("selected")).toBool();
        const QJsonObject endpoint = entry.value(QStringLiteral("serviceEndpoint")).toObject();
        const QJsonObject edit = endpoint.value(QStringLiteral("playlistEditEndpoint")).toObject();
        for (const QJsonValue &action : edit.value(QStringLiteral("actions")).toArray()) {
            const QJsonObject fields = action.toObject();
            if (fields.contains(QStringLiteral("playlistVideoOrder")))
                option.videoOrder = fields.value(QStringLiteral("playlistVideoOrder")).toInt();
            if (fields.contains(QStringLiteral("playlistDynamicSortPreference")))
                option.dynamicSort =
                    fields.value(QStringLiteral("playlistDynamicSortPreference")).toInt();
        }
        option.params = QUrl::fromPercentEncoding(
            findFirst(endpoint.value(QStringLiteral("browseEndpoint")), QStringLiteral("params"))
                .toString()
                .toUtf8());
        if (!option.title.isEmpty() && option.valid())
            options.append(option);
    }
    return options.isEmpty() ? readBrowseOptions(node) : options;
}

model::Item searchDestination(const QJsonValue &node)
{
    const QJsonObject search = findFirst(node, QStringLiteral("searchEndpoint")).toObject();
    model::Item item;
    if (search.isEmpty())
        return item;
    item.title = search.value(QStringLiteral("query")).toString();
    item.params = search.value(QStringLiteral("params")).toString();
    item.kind = QStringLiteral("search");
    return item;
}

model::Shelf readMessage(const QJsonObject &renderer)
{
    model::Shelf shelf;
    shelf.message = readText(renderer.value(QStringLiteral("text")));
    return shelf;
}

model::Shelf readShelf(const QString &key, const QJsonObject &renderer)
{
    model::Shelf shelf;
    shelf.horizontal =
        key == QLatin1String("musicCarouselShelfRenderer") || key.startsWith(QLatin1String("grid"));
    const QJsonObject header = renderer.value(QStringLiteral("header")).toObject();
    shelf.title = readText(renderer.value(QStringLiteral("title")));
    if (shelf.title.isEmpty())
        shelf.title = readText(findFirst(withoutSortMenu(header), QStringLiteral("title")));
    shelf.subtitle = readText(findFirst(header, QStringLiteral("strapline")));
    QJsonValue contents = renderer.value(QStringLiteral("contents"));
    if (contents.isUndefined())
        contents = renderer.value(QStringLiteral("items"));
    collectItems(contents, shelf.items);
    const QJsonValue message = findFirst(contents, QStringLiteral("messageRenderer"));
    if (shelf.items.isEmpty() && message.isObject())
        shelf.message = readMessage(message.toObject()).message;
    shelf.ranked =
        std::ranges::any_of(shelf.items, [](const model::Item &item) { return item.rank > 0; });
    shelf.navigation =
        !shelf.items.isEmpty() && std::ranges::all_of(shelf.items, [](const model::Item &item) {
        return item.kind == QLatin1String("category") && item.artId.isEmpty();
    });
    shelf.continuation = readContinuation(renderer);
    const QJsonObject more = findFirst(header, QStringLiteral("moreContentButton")).toObject();
    shelf.more = readItem(findFirst(more, QStringLiteral("buttonRenderer")).toObject());
    const QJsonObject bottom = renderer.value(QStringLiteral("bottomEndpoint")).toObject();
    if (shelf.more.browseId.isEmpty()) {
        shelf.more = readItem({{QStringLiteral("title"), shelf.title},
                               {QStringLiteral("navigationEndpoint"), bottom}});
    }
    if (!shelf.more.browseId.isEmpty())
        shelf.more.title = shelf.title;
    else
        shelf.more = searchDestination(bottom);
    return shelf;
}

model::Shelf readCard(const QJsonObject &renderer)
{
    model::Shelf shelf;
    shelf.card = true;
    shelf.lead = readItem(renderer);
    collectItems(renderer.value(QStringLiteral("contents")), shelf.items);
    for (model::Item &child : shelf.items) {
        if (shelf.lead.circular() && child.playable() && child.track.artist.isEmpty())
            child.track.artist = shelf.lead.title;
    }
    return shelf;
}

model::Shelf readDescription(const QJsonObject &renderer)
{
    model::Shelf shelf;
    shelf.title = readText(renderer.value(QStringLiteral("header")));
    shelf.subtitle = readText(renderer.value(QStringLiteral("strapline")));
    shelf.description = readText(renderer.value(QStringLiteral("description")));
    return shelf;
}

QList<QJsonObject> chipRenderers(const QJsonValue &node)
{
    QList<QJsonObject> renderers;
    const QJsonObject cloud = findFirst(node, QStringLiteral("chipCloudRenderer")).toObject();
    for (const QJsonValue &value : cloud.value(QStringLiteral("chips")).toArray()) {
        const QJsonObject renderer =
            value.toObject().value(QStringLiteral("chipCloudChipRenderer")).toObject();
        if (!renderer.isEmpty())
            renderers.append(renderer);
    }
    return renderers;
}

QJsonArray chipMenu(const QJsonObject &renderer)
{
    return findFirst(renderer.value(QStringLiteral("navigationEndpoint")),
                     QStringLiteral("menuPopupRenderer"))
        .toObject()
        .value(QStringLiteral("items"))
        .toArray();
}

model::Chip readChip(const QJsonObject &renderer)
{
    model::Chip chip;
    chip.title = readText(renderer.value(QStringLiteral("text")));
    chip.selected = renderer.value(QStringLiteral("isSelected")).toBool();
    if (chip.title.isEmpty())
        chip.icon = QStringLiteral("close");
    const QJsonValue endpoint = renderer.value(QStringLiteral("navigationEndpoint"));
    const QJsonObject search = findFirst(endpoint, QStringLiteral("searchEndpoint")).toObject();
    const QJsonObject browse = findFirst(endpoint, QStringLiteral("browseEndpoint")).toObject();
    chip.params = search.isEmpty() ? browse.value(QStringLiteral("params")).toString()
                                   : search.value(QStringLiteral("params")).toString();
    chip.browseId = browse.value(QStringLiteral("browseId")).toString();
    chip.continuation = reloadToken(endpoint);
    chip.deselection = reloadToken(renderer.value(QStringLiteral("onDeselectedCommand")));
    return chip;
}

QList<model::Chip> readChips(const QJsonValue &node)
{
    QList<model::Chip> chips;
    for (const QJsonObject &renderer : chipRenderers(node)) {
        if (chipMenu(renderer).isEmpty())
            chips.append(readChip(renderer));
    }
    return chips;
}

QList<model::SortOption> readChipSort(const QJsonValue &node)
{
    QList<model::SortOption> options;
    for (const QJsonObject &renderer : chipRenderers(node)) {
        for (const QJsonValue &value : chipMenu(renderer)) {
            const QJsonObject entry =
                value.toObject().value(QStringLiteral("menuNavigationItemRenderer")).toObject();
            model::SortOption option;
            option.title = readText(entry.value(QStringLiteral("text")));
            option.selected =
                findFirst(entry.value(QStringLiteral("icon")), QStringLiteral("iconType"))
                    .toString()
                == QLatin1String("CHECK");
            option.continuation = reloadToken(entry.value(QStringLiteral("navigationEndpoint")));
            if (!option.title.isEmpty() && option.valid())
                options.append(option);
        }
    }
    return options;
}

QList<model::Chip> readScopes(const QJsonObject &response)
{
    QList<model::Chip> scopes;
    const QJsonObject tabbed =
        findFirst(response, QStringLiteral("tabbedSearchResultsRenderer")).toObject();
    for (const QJsonValue &value : tabbed.value(QStringLiteral("tabs")).toArray()) {
        const QJsonObject tab = value.toObject().value(QStringLiteral("tabRenderer")).toObject();
        model::Chip scope;
        scope.title = tab.value(QStringLiteral("title")).toString();
        scope.selected = tab.value(QStringLiteral("selected")).toBool();
        scope.icon = kScopeGlyphs.value(tab.value(QStringLiteral("tabIdentifier")).toString(),
                                        QStringLiteral("search"));
        scope.params =
            findFirst(tab.value(QStringLiteral("endpoint")), QStringLiteral("params")).toString();
        if (!scope.title.isEmpty())
            scopes.append(scope);
    }
    return scopes;
}

QJsonArray visibleTabs(const QJsonObject &tabbed)
{
    const QJsonArray tabs = tabbed.value(QStringLiteral("tabs")).toArray();
    for (const QJsonValue &tab : tabs) {
        if (tab.toObject()
                .value(QStringLiteral("tabRenderer"))
                .toObject()
                .value(QStringLiteral("selected"))
                .toBool())
            return QJsonArray {tab};
    }
    return tabs;
}

QJsonValue pageHeader(const QJsonObject &response)
{
    const QJsonObject continued = response.value(QStringLiteral("continuationContents"))
                                      .toObject()
                                      .value(QStringLiteral("sectionListContinuation"))
                                      .toObject();
    if (!continued.isEmpty())
        return continued.value(QStringLiteral("header"));
    QJsonValue contents = response.value(QStringLiteral("contents"));
    const QJsonObject tabbed =
        findFirst(contents, QStringLiteral("tabbedSearchResultsRenderer")).toObject();
    if (!tabbed.isEmpty())
        contents = visibleTabs(tabbed);
    const QJsonValue sections = findFirst(contents, QStringLiteral("sectionListRenderer"));
    return sections.toObject().value(QStringLiteral("header"));
}

class Walker
{
public:
    explicit Walker(innertube::parsers::Page &page)
        : m_page(page)
    {
    }

    void walk(const QJsonValue &node)
    {
        if (node.isArray()) {
            for (const QJsonValue &value : node.toArray())
                walk(value);
            return;
        }
        const QJsonObject object = node.toObject();
        for (auto it = object.begin(); it != object.end(); ++it) {
            const QString &key = it.key();
            const QJsonObject renderer = it.value().toObject();
            if (kItems.contains(key)) {
                const model::Item item = readItem(renderer);
                if (item.valid())
                    m_loose.append(item);
                return;
            }
            if (kShelves.contains(key)) {
                model::Shelf shelf = readShelf(key, renderer);
                if (!shelf.items.isEmpty() || !shelf.continuation.isEmpty()
                    || !shelf.message.isEmpty() || shelf.more.valid())
                    add(std::move(shelf));
                return;
            }
            if (key == QLatin1String("tabbedSearchResultsRenderer")) {
                walk(visibleTabs(renderer));
                return;
            }
            if (key == QLatin1String("musicCardShelfRenderer")) {
                add(readCard(renderer));
                return;
            }
            if (key == QLatin1String("musicDescriptionShelfRenderer")) {
                model::Shelf shelf = readDescription(renderer);
                if (!shelf.description.isEmpty())
                    add(std::move(shelf));
                return;
            }
            if (key == QLatin1String("messageRenderer")) {
                model::Shelf shelf = readMessage(renderer);
                if (!shelf.message.isEmpty())
                    add(std::move(shelf));
                return;
            }
            if (key == QLatin1String("sectionListRenderer")
                || key == QLatin1String("sectionListContinuation")
                || key == QLatin1String("appendContinuationItemsAction")) {
                const QString token = readContinuation(renderer);
                if (!token.isEmpty())
                    m_page.continuation = token;
            }
            if (key.endsWith(QLatin1String("Renderer")) && !kWrappers.contains(key)) {
                qCDebug(logInnerTube) << "skipping renderer" << key;
                continue;
            }
            if (key == QLatin1String("header") || key == QLatin1String("menu")
                || key == QLatin1String("navigationEndpoint") || key == QLatin1String("overlay"))
                continue;
            walk(it.value());
        }
    }

    void finish() { flush(); }

private:
    static bool untitledList(const model::Shelf &shelf)
    {
        return shelf.title.isEmpty() && !shelf.horizontal && !shelf.card
            && shelf.continuation.isEmpty() && shelf.message.isEmpty()
            && shelf.description.isEmpty() && !shelf.items.isEmpty();
    }

    void add(model::Shelf shelf)
    {
        flush();
        if (untitledList(shelf) && !m_page.shelves.isEmpty()
            && untitledList(m_page.shelves.last())) {
            m_page.shelves.last().items.append(shelf.items);
            return;
        }
        m_page.shelves.append(std::move(shelf));
    }

    void flush()
    {
        if (m_loose.isEmpty())
            return;
        model::Shelf shelf;
        shelf.items = std::exchange(m_loose, {});
        if (!m_page.shelves.isEmpty() && untitledList(m_page.shelves.last())) {
            m_page.shelves.last().items.append(shelf.items);
            return;
        }
        m_page.shelves.append(std::move(shelf));
    }

    innertube::parsers::Page &m_page;
    QList<model::Item> m_loose;
};

}

namespace innertube::parsers {

Page RendererParser::parse(const QJsonObject &response)
{
    Page page;
    page.header = readHeader(response);
    const QJsonValue header = pageHeader(response);
    page.sortOptions = readSortOptions(response);
    if (page.sortOptions.isEmpty())
        page.sortOptions = readChipSort(header);
    page.scopes = readScopes(response);
    page.chips = readChips(header);
    Walker walker(page);
    walker.walk(response);
    walker.finish();
    return page;
}

}
