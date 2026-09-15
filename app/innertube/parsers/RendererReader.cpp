#include "RendererReader.h"

#include "core/Json.h"

#include <QHash>
#include <QJsonArray>
#include <QRegularExpression>
#include <QSet>

#include <algorithm>

namespace {

const QString kUploadedTrackType = QStringLiteral("MUSIC_VIDEO_TYPE_PRIVATELY_OWNED_TRACK");
const QString kEpisodeTrackType = QStringLiteral("MUSIC_VIDEO_TYPE_PODCAST_EPISODE");
const QString kEpisodePageType = QStringLiteral("MUSIC_PAGE_TYPE_NON_MUSIC_AUDIO_TRACK_PAGE");
const QString kPodcastPageType = QStringLiteral("MUSIC_PAGE_TYPE_PODCAST_SHOW_DETAIL_PAGE");
const QString kProfilePageType = QStringLiteral("MUSIC_PAGE_TYPE_USER_CHANNEL");
const QString kLaterPlaylist = QStringLiteral("SE");

QString kindOf(const QJsonObject &browse)
{
    using innertube::parsers::findFirst;
    const QString type = findFirst(browse, QStringLiteral("pageType")).toString();
    const QString id = browse.value(QStringLiteral("browseId")).toString();
    if (type == kPodcastPageType || id.startsWith(QLatin1String("MPSP")))
        return QStringLiteral("podcast");
    if (type == kEpisodePageType || id.startsWith(QLatin1String("MPED")))
        return QStringLiteral("episode");
    if (type == kProfilePageType)
        return QStringLiteral("profile");
    if (type.contains(QLatin1String("ARTIST")) || id.startsWith(QLatin1String("UC")))
        return QStringLiteral("artist");
    if (type.contains(QLatin1String("ALBUM")) || id.startsWith(QLatin1String("MPRE")))
        return QStringLiteral("album");
    if (type.contains(QLatin1String("PLAYLIST")) || id.startsWith(QLatin1String("VL")))
        return QStringLiteral("playlist");
    return QStringLiteral("category");
}

const QStringList kLiveBadgeStyles {
    QStringLiteral("LIVE"),
    QStringLiteral("BADGE_STYLE_TYPE_LIVE_NOW"),
    QStringLiteral("BADGE_LIVE_NOW"),
};
const QString kPinnedBadgeIcon = QStringLiteral("KEEP");
const QString kPlayedIcon = QStringLiteral("CHECK");
const QString kUnpinnedBadgeIcon = QStringLiteral("KEEP_OFF");

const QSet<QString> kLibraryAddIcons {
    QStringLiteral("BOOKMARK_BORDER"),
    QStringLiteral("LIBRARY_ADD"),
};

const QSet<QString> kLibraryRemoveIcons {
    QStringLiteral("BOOKMARK"),
    QStringLiteral("LIBRARY_REMOVE"),
    QStringLiteral("LIBRARY_SAVED"),
};

const QHash<QString, QString> kNavigationGlyphs {
    {QStringLiteral("MUSIC_NEW_RELEASE"), QStringLiteral("album")},
    {QStringLiteral("TRENDING_UP"), QStringLiteral("trending_up")},
    {QStringLiteral("STICKER_EMOTICON"), QStringLiteral("mood")},
    {QStringLiteral("MUSIC_VIDEO"), QStringLiteral("music_video")},
    {QStringLiteral("PODCASTS"), QStringLiteral("podcasts")},
};

QString iconOf(const QJsonValue &node)
{
    return innertube::parsers::findFirst(node, QStringLiteral("iconType")).toString();
}

QString feedbackTokenOf(const QJsonValue &endpoint)
{
    using innertube::parsers::findFirst;
    const QJsonValue feedback = findFirst(endpoint, QStringLiteral("feedbackEndpoint"));
    return findFirst(feedback, QStringLiteral("feedbackToken")).toString();
}

bool readLaterToggle(const QJsonObject &toggle, model::Item &item)
{
    using innertube::parsers::findFirst;
    const QJsonObject edit = findFirst(toggle, QStringLiteral("playlistEditEndpoint")).toObject();
    if (edit.value(QStringLiteral("playlistId")).toString() != kLaterPlaylist)
        return false;
    item.actions.laterable = true;
    item.actions.later = toggle.value(QStringLiteral("isToggled")).toBool();
    return true;
}

bool readSaveToggle(const QJsonObject &toggle, model::Item &item)
{
    using innertube::parsers::findFirst;
    const QJsonObject like = findFirst(toggle, QStringLiteral("likeEndpoint")).toObject();
    const QString target = findFirst(like, QStringLiteral("playlistId")).toString();
    if (target.isEmpty())
        return false;
    item.savePlaylistId = target;
    item.saved = toggle.value(QStringLiteral("isToggled")).toBool();
    return true;
}

void readToggleAction(const QJsonObject &toggle, model::Item &item)
{
    if (readLaterToggle(toggle, item) || readSaveToggle(toggle, item))
        return;
    const QString defaultToken =
        feedbackTokenOf(toggle.value(QStringLiteral("defaultServiceEndpoint")));
    const QString toggledToken =
        feedbackTokenOf(toggle.value(QStringLiteral("toggledServiceEndpoint")));
    if (defaultToken.isEmpty() && toggledToken.isEmpty())
        return;
    const QString icon = iconOf(toggle.value(QStringLiteral("defaultIcon")));
    if (icon == kPlayedIcon) {
        item.actions.playedToken = defaultToken;
        item.actions.unplayedToken = toggledToken;
        item.played = toggle.value(QStringLiteral("isToggled")).toBool();
    } else if (icon == kPinnedBadgeIcon) {
        item.actions.pinToken = defaultToken;
        item.actions.unpinToken = toggledToken;
    } else if (icon == kUnpinnedBadgeIcon) {
        item.actions.unpinToken = defaultToken;
        item.actions.pinToken = toggledToken;
        item.pinned = true;
    } else if (kLibraryAddIcons.contains(icon)) {
        item.actions.libraryAddToken = defaultToken;
        item.actions.libraryRemoveToken = toggledToken;
        item.actions.inLibrary = false;
    } else if (kLibraryRemoveIcons.contains(icon)) {
        item.actions.libraryRemoveToken = defaultToken;
        item.actions.libraryAddToken = toggledToken;
        item.actions.inLibrary = true;
    }
}

void readNavigationAction(const QJsonObject &entry, model::Item &item)
{
    using innertube::parsers::findFirst;
    const QJsonObject endpoint = entry.value(QStringLiteral("navigationEndpoint")).toObject();
    QJsonObject watch = endpoint.value(QStringLiteral("watchEndpoint")).toObject();
    if (watch.isEmpty())
        watch = endpoint.value(QStringLiteral("watchPlaylistEndpoint")).toObject();
    const QString icon = iconOf(entry.value(QStringLiteral("icon")));
    if (icon == QLatin1String("MIX")) {
        item.actions.mixPlaylistId = watch.value(QStringLiteral("playlistId")).toString();
        item.actions.mixVideoId = watch.value(QStringLiteral("videoId")).toString();
    } else if (icon == QLatin1String("MUSIC_SHUFFLE")) {
        item.actions.shufflePlaylistId = watch.value(QStringLiteral("playlistId")).toString();
    } else if (icon == QLatin1String("ARTIST")) {
        item.actions.artistId = findFirst(endpoint, QStringLiteral("browseId")).toString();
    } else if (icon == QLatin1String("ALBUM")) {
        item.actions.albumId = findFirst(endpoint, QStringLiteral("browseId")).toString();
    } else if (icon == QLatin1String("EDIT")) {
        item.actions.editable = true;
    } else if (icon == QLatin1String("INFO")) {
        item.actions.episodeId = findFirst(endpoint, QStringLiteral("browseId")).toString();
    } else if (icon == QLatin1String("BROADCAST")) {
        item.actions.podcastId = findFirst(endpoint, QStringLiteral("browseId")).toString();
    }
}

void readServiceAction(const QJsonObject &entry, model::Item &item)
{
    using innertube::parsers::findFirst;
    if (iconOf(entry.value(QStringLiteral("icon"))) != QLatin1String("REMOVE_FROM_PLAYLIST"))
        return;
    const QJsonObject edit = findFirst(entry.value(QStringLiteral("serviceEndpoint")),
                                       QStringLiteral("playlistEditEndpoint"))
                                 .toObject();
    item.actions.sourcePlaylistId = edit.value(QStringLiteral("playlistId")).toString();
    for (const QJsonValue &value : edit.value(QStringLiteral("actions")).toArray()) {
        const QJsonObject action = value.toObject();
        if (action.value(QStringLiteral("action")).toString()
            == QLatin1String("ACTION_REMOVE_VIDEO"))
            item.actions.setVideoId = action.value(QStringLiteral("setVideoId")).toString();
    }
}

void readMenuActions(const QJsonValue &menu, model::Item &item)
{
    using innertube::parsers::findFirst;
    const QJsonArray entries = findFirst(menu, QStringLiteral("items")).toArray();
    for (const QJsonValue &value : entries) {
        const QJsonObject wrapper = value.toObject();
        for (auto it = wrapper.begin(); it != wrapper.end(); ++it) {
            const QJsonObject entry = it.value().toObject();
            if (it.key() == QLatin1String("toggleMenuServiceItemRenderer"))
                readToggleAction(entry, item);
            else if (it.key() == QLatin1String("menuNavigationItemRenderer"))
                readNavigationAction(entry, item);
            else if (it.key() == QLatin1String("menuServiceItemRenderer"))
                readServiceAction(entry, item);
        }
    }
}

void readEditHeader(const QJsonObject &root, model::Item &item)
{
    using innertube::parsers::findFirst;
    using innertube::parsers::readText;
    const QJsonObject header =
        findFirst(root, QStringLiteral("musicPlaylistEditHeaderRenderer")).toObject();
    if (header.isEmpty())
        return;
    item.owned = true;
    item.actions.editable = true;
    item.privacy = header.value(QStringLiteral("privacy")).toString();
    item.description = readText(header.value(QStringLiteral("description")));
}

void readNavigationStyle(const QJsonObject &renderer, model::Item &item)
{
    const QJsonObject solid = renderer.value(QStringLiteral("solid")).toObject();
    item.striped = solid.contains(QStringLiteral("leftStripeColor"));
    item.glyph = kNavigationGlyphs.value(iconOf(renderer.value(QStringLiteral("iconStyle"))));
}

bool readPinned(const QJsonObject &renderer)
{
    using innertube::parsers::findFirst;
    for (const QJsonValue &value : renderer.value(QStringLiteral("subtitleBadges")).toArray()) {
        const QJsonObject badge =
            value.toObject().value(QStringLiteral("musicInlineBadgeRenderer")).toObject();
        if (findFirst(badge, QStringLiteral("iconType")).toString() == kPinnedBadgeIcon)
            return true;
    }
    return false;
}

void readHeaderToggles(const QJsonValue &buttons, model::Item &item)
{
    for (const QJsonValue &value : buttons.toArray()) {
        const QJsonObject toggle =
            value.toObject().value(QStringLiteral("toggleButtonRenderer")).toObject();
        if (!toggle.isEmpty() && !readLaterToggle(toggle, item))
            readSaveToggle(toggle, item);
    }
}

QJsonObject episodeWatch(const QJsonObject &renderer)
{
    using innertube::parsers::findFirst;
    for (const QString &key : {QStringLiteral("overlay"), QStringLiteral("thumbnailOverlay"),
                               QStringLiteral("buttons")}) {
        const QJsonObject watch =
            findFirst(renderer.value(key), QStringLiteral("watchEndpoint")).toObject();
        if (!watch.value(QStringLiteral("videoId")).toString().isEmpty())
            return watch;
    }
    return {};
}

bool readLive(const QJsonObject &renderer)
{
    using innertube::parsers::containsText;
    using innertube::parsers::findFirst;
    for (const QString &key :
         {QStringLiteral("badges"), QStringLiteral("subtitleBadges"),
          QStringLiteral("thumbnailOverlay"), QStringLiteral("thumbnailOverlays"),
          QStringLiteral("overlay"), QStringLiteral("thumbnailRenderer")}) {
        const QJsonValue node = renderer.value(key);
        if (!findFirst(node, QStringLiteral("liveBadgeRenderer")).isUndefined()
            || !findFirst(node, QStringLiteral("liveBadge")).isUndefined()
            || containsText(node, QStringLiteral("iconType"), QStringLiteral("LIVE"))
            || std::ranges::any_of(kLiveBadgeStyles, [&node](const QString &style) {
            return containsText(node, QStringLiteral("style"), style);
        }))
            return true;
    }
    return false;
}

bool readUpload(const QJsonObject &renderer)
{
    using innertube::parsers::findFirst;
    return findFirst(renderer, QStringLiteral("musicVideoType")).toString() == kUploadedTrackType;
}

qint64 durationOf(const QString &text)
{
    static const QRegularExpression duration(QStringLiteral("^(?:[0-9]+:)?[0-9]+:[0-5][0-9]$"));
    if (!duration.match(text).hasMatch())
        return 0;
    qint64 seconds = 0;
    for (const QString &part : text.split(QLatin1Char(':')))
        seconds = seconds * 60 + part.toLongLong();
    return seconds * 1000;
}

void readMetadata(const QJsonValue &text, model::Item &item)
{
    using namespace innertube::parsers;
    QStringList artists;
    for (const QJsonValue &value : text.toObject().value(QStringLiteral("runs")).toArray()) {
        const QJsonObject run = value.toObject();
        const QJsonObject browse = findFirst(run.value(QStringLiteral("navigationEndpoint")),
                                             QStringLiteral("browseEndpoint"))
                                       .toObject();
        const QString label = run.value(QStringLiteral("text")).toString();
        if (!browse.isEmpty()) {
            const QString kind = kindOf(browse);
            const bool credited =
                kind == QLatin1String("artist") || kind == QLatin1String("podcast");
            if (credited && !artists.contains(label))
                artists.append(label);
            if (kind == QLatin1String("album"))
                item.track.album = label;
            if (kind == QLatin1String("podcast"))
                item.actions.podcastId = browse.value(QStringLiteral("browseId")).toString();
            if (kind == QLatin1String("profile"))
                item.actions.profileId = browse.value(QStringLiteral("browseId")).toString();
        }
        const qint64 duration = durationOf(label.trimmed());
        if (duration > 0)
            item.track.durationMs = duration;
    }
    if (!artists.isEmpty())
        item.track.artist = artists.join(QStringLiteral(", "));
    const qint64 duration = durationOf(readText(text));
    if (duration > 0)
        item.track.durationMs = duration;
}

QString withoutSeparator(const QString &text)
{
    static const QString separators = QStringLiteral(" ") + QChar(0x2022);
    qsizetype start = 0;
    while (start < text.size() && separators.contains(text.at(start)))
        ++start;
    return text.mid(start).trimmed();
}

void readProgress(const QJsonValue &node, model::Item &item)
{
    using namespace innertube::parsers;
    const QJsonObject progress =
        findFirst(node, QStringLiteral("musicPlaybackProgressRenderer")).toObject();
    if (progress.isEmpty())
        return;
    item.progress = int(qBound(
        qint64(0), core::json::toInt(progress.value(QStringLiteral("playbackProgressPercentage"))),
        qint64(100)));
    item.progressLabel =
        withoutSeparator(readText(progress.value(QStringLiteral("playbackProgressText"))));
    if (item.actions.markable())
        return;
    item.played = item.progress >= 100;
}

void readButtonActions(const QJsonValue &buttons, model::Item &item)
{
    for (const QJsonValue &value : buttons.toArray()) {
        const QJsonObject button =
            value.toObject().value(QStringLiteral("buttonRenderer")).toObject();
        if (button.isEmpty())
            continue;
        readNavigationAction(
            {{QStringLiteral("icon"), button.value(QStringLiteral("icon"))},
             {QStringLiteral("navigationEndpoint"), button.value(QStringLiteral("command"))}},
            item);
    }
}

void readByline(const QJsonObject &header, model::Item &item)
{
    using namespace innertube::parsers;
    const QJsonValue strapline = header.value(QStringLiteral("straplineTextOne"));
    item.byline = readText(strapline);
    item.bylineArtId = readThumbnail(header.value(QStringLiteral("straplineThumbnail")));
    const QJsonObject browse = findFirst(strapline, QStringLiteral("browseEndpoint")).toObject();
    if (browse.isEmpty())
        return;
    const QString kind = kindOf(browse);
    const QString id = browse.value(QStringLiteral("browseId")).toString();
    if (kind == QLatin1String("profile"))
        item.actions.profileId = id;
    else if (kind == QLatin1String("podcast"))
        item.actions.podcastId = id;
    else if (item.actions.artistId.isEmpty())
        item.actions.artistId = id;
}

void readHeaderPlayback(const QJsonObject &header, model::Item &item)
{
    using namespace innertube::parsers;
    readProgress(header.value(QStringLiteral("progress")), item);
    if (item.track.valid())
        return;
    const QJsonObject play =
        findFirst(header.value(QStringLiteral("buttons")), QStringLiteral("playNavigationEndpoint"))
            .toObject();
    const QJsonObject watch = play.value(QStringLiteral("watchEndpoint")).toObject();
    item.track.videoId = watch.value(QStringLiteral("videoId")).toString();
    item.track.episode = item.track.valid()
        && findFirst(watch, QStringLiteral("musicVideoType")).toString() == kEpisodeTrackType;
    item.track.title = item.title;
    item.track.artId = item.artId;
    if (item.track.episode && item.track.artist.isEmpty())
        item.track.artist = item.byline;
}

}

namespace innertube::parsers {

QJsonValue findFirst(const QJsonValue &root, const QString &key)
{
    if (root.isObject()) {
        const QJsonObject object = root.toObject();
        if (object.contains(key))
            return object.value(key);
        for (auto it = object.begin(); it != object.end(); ++it) {
            const QJsonValue found = findFirst(it.value(), key);
            if (!found.isUndefined())
                return found;
        }
    } else if (root.isArray()) {
        for (const QJsonValue &value : root.toArray()) {
            const QJsonValue found = findFirst(value, key);
            if (!found.isUndefined())
                return found;
        }
    }
    return QJsonValue(QJsonValue::Undefined);
}

bool containsText(const QJsonValue &root, const QString &key, const QString &value)
{
    if (root.isObject()) {
        const QJsonObject object = root.toObject();
        for (auto it = object.begin(); it != object.end(); ++it) {
            if (it.key() == key && it.value().toString() == value)
                return true;
            if (containsText(it.value(), key, value))
                return true;
        }
    } else if (root.isArray()) {
        for (const QJsonValue &entry : root.toArray()) {
            if (containsText(entry, key, value))
                return true;
        }
    }
    return false;
}

QString readText(const QJsonValue &node)
{
    if (node.isString())
        return node.toString();
    const QJsonObject object = node.toObject();
    if (object.contains(QStringLiteral("simpleText")))
        return object.value(QStringLiteral("simpleText")).toString();
    QString text;
    for (const QJsonValue &run : object.value(QStringLiteral("runs")).toArray())
        text += run.toObject().value(QStringLiteral("text")).toString();
    return text.trimmed();
}

QString readThumbnail(const QJsonValue &node)
{
    const QJsonArray thumbnails = findFirst(node, QStringLiteral("thumbnails")).toArray();
    QString url;
    qint64 largest = -1;
    for (const QJsonValue &value : thumbnails) {
        const QJsonObject thumbnail = value.toObject();
        const QString candidate = thumbnail.value(QStringLiteral("url")).toString();
        const qint64 width = core::json::toInt(thumbnail.value(QStringLiteral("width")));
        if (!candidate.isEmpty() && width >= largest) {
            largest = width;
            url = candidate;
        }
    }
    return url;
}

QString readContinuation(const QJsonObject &container)
{
    for (const QJsonValue &value : container.value(QStringLiteral("continuations")).toArray()) {
        const QString token = findFirst(value, QStringLiteral("nextContinuationData"))
                                  .toObject()
                                  .value(QStringLiteral("continuation"))
                                  .toString();
        if (!token.isEmpty())
            return token;
    }
    for (const QString &key : {QStringLiteral("contents"), QStringLiteral("items"),
                               QStringLiteral("continuationItems")}) {
        for (const QJsonValue &value : container.value(key).toArray()) {
            const QJsonValue renderer =
                value.toObject().value(QStringLiteral("continuationItemRenderer"));
            const QString token = findFirst(renderer, QStringLiteral("continuationCommand"))
                                      .toObject()
                                      .value(QStringLiteral("token"))
                                      .toString();
            if (!token.isEmpty())
                return token;
        }
    }
    return {};
}

model::Item readItem(const QJsonObject &renderer)
{
    model::Item item;
    QJsonValue title = renderer.value(QStringLiteral("title"));
    QJsonValue subtitle = renderer.value(QStringLiteral("subtitle"));
    const QJsonArray columns = renderer.value(QStringLiteral("flexColumns")).toArray();
    if (!columns.isEmpty()) {
        title = findFirst(columns.at(0), QStringLiteral("text"));
        if (columns.size() > 1)
            subtitle = findFirst(columns.at(1), QStringLiteral("text"));
    }
    if (title.isUndefined())
        title = renderer.value(QStringLiteral("buttonText"));
    if (title.isUndefined())
        title = renderer.value(QStringLiteral("text"));
    const QJsonObject ranking = findFirst(renderer.value(QStringLiteral("customIndexColumn")),
                                          QStringLiteral("musicCustomIndexColumnRenderer"))
                                    .toObject();
    item.rank = readText(ranking.value(QStringLiteral("text"))).toInt();
    const QString movement = iconOf(ranking.value(QStringLiteral("icon")));
    if (movement == QLatin1String("ARROW_DROP_UP"))
        item.movement = QStringLiteral("up");
    else if (movement == QLatin1String("ARROW_DROP_DOWN"))
        item.movement = QStringLiteral("down");
    else if (movement == QLatin1String("ARROW_CHART_NEUTRAL"))
        item.movement = QStringLiteral("steady");
    item.rankLabel =
        findFirst(ranking.value(QStringLiteral("accessibilityData")), QStringLiteral("label"))
            .toString();
    item.title = readText(title);
    item.subtitle = readText(subtitle);

    QJsonObject endpoint = renderer.value(QStringLiteral("navigationEndpoint")).toObject();
    if (endpoint.isEmpty())
        endpoint = findFirst(title, QStringLiteral("navigationEndpoint")).toObject();
    if (endpoint.isEmpty())
        endpoint = renderer.value(QStringLiteral("clickCommand")).toObject();
    if (endpoint.isEmpty())
        endpoint = renderer.value(QStringLiteral("onTap")).toObject();

    QJsonObject browse = endpoint.value(QStringLiteral("browseEndpoint")).toObject();
    QJsonObject watch = endpoint.value(QStringLiteral("watchEndpoint")).toObject();
    if (findFirst(browse, QStringLiteral("pageType")).toString() == kEpisodePageType) {
        const QJsonObject episode = episodeWatch(renderer);
        if (!episode.isEmpty()) {
            item.actions.episodeId = browse.value(QStringLiteral("browseId")).toString();
            watch = episode;
            browse = {};
        }
    }
    if (watch.isEmpty() && browse.isEmpty())
        watch =
            findFirst(renderer.value(QStringLiteral("overlay")), QStringLiteral("watchEndpoint"))
                .toObject();
    if (watch.isEmpty())
        watch = endpoint.value(QStringLiteral("watchPlaylistEndpoint")).toObject();

    item.browseId = browse.value(QStringLiteral("browseId")).toString();
    item.params = browse.value(QStringLiteral("params")).toString();
    item.playlistId = watch.value(QStringLiteral("playlistId")).toString();
    if (browse.isEmpty())
        item.params = watch.value(QStringLiteral("params")).toString();
    item.track.videoId = renderer.value(QStringLiteral("playlistItemData"))
                             .toObject()
                             .value(QStringLiteral("videoId"))
                             .toString();
    if (item.track.videoId.isEmpty())
        item.track.videoId = renderer.value(QStringLiteral("videoId")).toString();
    if (item.track.videoId.isEmpty())
        item.track.videoId = watch.value(QStringLiteral("videoId")).toString();

    item.kind = !browse.isEmpty() ? kindOf(browse)
        : item.track.valid()      ? QStringLiteral("song")
                                  : QStringLiteral("playlist");
    for (const QString &key : {QStringLiteral("thumbnailRenderer"), QStringLiteral("thumbnail")}) {
        item.artId = readThumbnail(renderer.value(key));
        if (!item.artId.isEmpty())
            break;
    }
    item.track.title = item.title;
    item.track.artId = item.artId;
    const QString rating = findFirst(renderer, QStringLiteral("likeStatus")).toString();
    item.track.liked = rating == QLatin1String("LIKE");
    item.track.disliked = rating == QLatin1String("DISLIKE");
    item.track.ratingKnown = !rating.isEmpty();
    const QString videoType = findFirst(renderer, QStringLiteral("musicVideoType")).toString();
    item.track.video = videoType == QLatin1String("MUSIC_VIDEO_TYPE_OMV")
        || videoType == QLatin1String("MUSIC_VIDEO_TYPE_UGC") || videoType == kEpisodeTrackType;
    item.track.upload = readUpload(renderer);
    item.track.episode = item.track.valid() && videoType == kEpisodeTrackType;
    if (item.track.episode && browse.isEmpty())
        item.kind = QStringLiteral("episode");
    item.track.live = item.track.valid() && readLive(renderer);
    item.pinned = readPinned(renderer);
    const QJsonValue menu = renderer.value(QStringLiteral("menu"));
    item.deletable = containsText(menu, QStringLiteral("iconType"), QStringLiteral("DELETE"));
    readMenuActions(menu, item);
    readButtonActions(renderer.value(QStringLiteral("buttons")), item);
    readNavigationStyle(renderer, item);
    const QString setVideoId = renderer.value(QStringLiteral("playlistItemData"))
                                   .toObject()
                                   .value(QStringLiteral("playlistSetVideoId"))
                                   .toString();
    if (!setVideoId.isEmpty())
        item.actions.setVideoId = setVideoId;
    readMetadata(subtitle, item);
    for (qsizetype i = 1; i < columns.size(); ++i)
        readMetadata(findFirst(columns.at(i), QStringLiteral("text")), item);
    for (const QJsonValue &column : renderer.value(QStringLiteral("fixedColumns")).toArray())
        readMetadata(findFirst(column, QStringLiteral("text")), item);
    readMetadata(renderer.value(QStringLiteral("lengthText")), item);
    readMetadata(renderer.value(QStringLiteral("longBylineText")), item);
    if (item.track.artist.isEmpty())
        item.track.artist = readText(renderer.value(QStringLiteral("shortBylineText")));
    if (item.subtitle.isEmpty())
        item.subtitle = item.track.artist;
    const QJsonValue description = renderer.value(QStringLiteral("description"));
    if (description.toObject().contains(QStringLiteral("runs")))
        item.description = readText(description);
    readProgress(renderer.value(QStringLiteral("playbackProgress")), item);
    return item;
}

model::Item readHeader(const QJsonObject &root)
{
    for (const QString &key : {QStringLiteral("musicResponsiveHeaderRenderer"),
                               QStringLiteral("musicDetailHeaderRenderer"),
                               QStringLiteral("musicImmersiveHeaderRenderer"),
                               QStringLiteral("musicVisualHeaderRenderer")}) {
        const QJsonObject header = findFirst(root, key).toObject();
        if (header.isEmpty())
            continue;
        model::Item item = readItem(header);
        const QJsonObject subscription =
            findFirst(header, QStringLiteral("subscribeButtonRenderer")).toObject();
        if (subscription.value(QStringLiteral("enabled")).toBool(true))
            item.channelId = subscription.value(QStringLiteral("channelId")).toString();
        item.subscribed = subscription.value(QStringLiteral("subscribed")).toBool();
        readHeaderToggles(header.value(QStringLiteral("buttons")), item);
        readMenuActions(
            findFirst(header.value(QStringLiteral("buttons")), QStringLiteral("menuRenderer")),
            item);
        readEditHeader(root, item);
        item.deletable = containsText(header, QStringLiteral("iconType"), QStringLiteral("DELETE"));
        readByline(header, item);
        readHeaderPlayback(header, item);
        const QJsonValue about = findFirst(header.value(QStringLiteral("description")),
                                           QStringLiteral("musicDescriptionShelfRenderer"));
        if (about.isObject())
            item.description = readText(about.toObject().value(QStringLiteral("description")));
        const QString second = readText(header.value(QStringLiteral("secondSubtitle")));
        if (!second.isEmpty())
            item.subtitle += (item.subtitle.isEmpty() ? QString() : QStringLiteral(" - ")) + second;
        if (item.track.artist.isEmpty())
            readMetadata(header.value(QStringLiteral("straplineTextOne")), item);
        return item;
    }
    return {};
}

}
