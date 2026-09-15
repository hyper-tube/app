#include "LibraryActions.h"

#include "auth/Account.h"
#include "core/Logging.h"
#include "innertube/parsers/RendererParser.h"
#include "innertube/parsers/RendererReader.h"
#include "net/Connectivity.h"

#include <QCoreApplication>
#include <QQmlEngine>
#include <QJsonArray>
#include <QJsonObject>

#include <optional>

namespace {

QString withoutListPrefix(const QString &playlistId)
{
    return playlistId.startsWith(QLatin1String("VL")) ? playlistId.mid(2) : playlistId;
}

QJsonObject videoTarget(const QString &videoId)
{
    return {{QStringLiteral("target"), QJsonObject {{QStringLiteral("videoId"), videoId}}}};
}

std::optional<model::Item> entryFor(const QString &videoId, const QJsonObject &response)
{
    const innertube::parsers::Page page = innertube::parsers::RendererParser::parse(response);
    for (const model::Shelf &shelf : page.shelves) {
        for (const model::Item &item : shelf.items) {
            if (item.track.videoId == videoId)
                return item;
        }
    }
    return std::nullopt;
}

int duplicatesRejected(const QJsonObject &response)
{
    const QJsonObject retry =
        innertube::parsers::findFirst(response.value(QStringLiteral("actions")),
                                      QStringLiteral("playlistEditEndpoint"))
            .toObject();
    int rejected = 0;
    for (const QJsonValue &value : retry.value(QStringLiteral("actions")).toArray()) {
        if (value.toObject().contains(QStringLiteral("dedupeOption")))
            ++rejected;
    }
    return rejected;
}

const QString kEditEndpoint = QStringLiteral("browse/edit_playlist");
const QString kLaterPlaylist = QStringLiteral("SE");
const QString kFeedbackEndpoint = QStringLiteral("feedback");
const QString kEditSucceeded = QStringLiteral("STATUS_SUCCEEDED");

}

namespace library {

LibraryActions::LibraryActions(QObject *parent)
    : QObject(parent)
    , m_endpoints(innertube::Session::instance())
{
    connect(&auth::Account::instance(), &auth::Account::changed, this, [this] {
        const auto &account = auth::Account::instance();
        const QString identity = account.signedIn() ? account.channelId() : QString();
        if (identity == m_accountId)
            return;
        m_accountId = identity;
        ++m_accountGeneration;
        forget();
    });
}

void LibraryActions::forget()
{
    m_ratings.clear();
    m_ratingVersions.clear();
    m_ratingWrites.clear();
    m_pins.clear();
    m_collected.clear();
    m_later.clear();
    m_played.clear();
    Q_EMIT pendingRatingsChanged();
}

void LibraryActions::applyOverrides(model::Item &item) const
{
    const QString key = item.key();
    if (key.isEmpty())
        return;
    const auto pin = m_pins.constFind(key);
    if (pin != m_pins.constEnd())
        item.pinned = *pin;
    const auto collected = m_collected.constFind(key);
    if (collected != m_collected.constEnd())
        item.actions.inLibrary = *collected;
    if (!item.playable())
        return;
    const auto later = m_later.constFind(key);
    if (later != m_later.constEnd())
        item.actions.later = *later;
    const auto played = m_played.constFind(key);
    if (played != m_played.constEnd())
        item.played = *played;
}

LibraryActions &LibraryActions::instance()
{
    static auto *actions = new LibraryActions(QCoreApplication::instance());
    return *actions;
}

LibraryActions *LibraryActions::create(QQmlEngine *, QJSEngine *)
{
    QQmlEngine::setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
    return &instance();
}

int LibraryActions::rating(const QString &videoId, int fallback) const
{
    return m_ratings.value(videoId, fallback);
}

void LibraryActions::rememberRating(const QString &videoId, int rating)
{
    if (!m_ratingWrites.contains(videoId))
        m_ratings.insert(videoId, rating);
}

void LibraryActions::refreshTrack(const QString &videoId)
{
    if (videoId.isEmpty() || !auth::Account::instance().signedIn()
        || !net::Connectivity::instance().online())
        return;
    const quint64 generation = m_accountGeneration;
    const quint64 version = m_ratingVersions.value(videoId);
    m_endpoints.next({}, videoId, {},
                     [this, videoId, generation, version](const innertube::Reply &reply) {
        if (!reply.ok() || generation != m_accountGeneration)
            return;
        if (const std::optional<model::Item> entry = entryFor(videoId, reply.json))
            Q_EMIT trackEntryRead(*entry);
        if (m_ratingWrites.contains(videoId) || m_ratingVersions.value(videoId) != version)
            return;
        const QString status =
            innertube::parsers::findFirst(reply.json.value(QStringLiteral("playerOverlays")),
                                          QStringLiteral("likeStatus"))
                .toString();
        if (status.isEmpty())
            return;
        const int value = status == QLatin1String("LIKE") ? 1
            : status == QLatin1String("DISLIKE")          ? -1
                                                          : 0;
        m_ratings.insert(videoId, value);
        Q_EMIT trackRatingChanged(videoId, value);
    });
}

void LibraryActions::setTrackLiked(const QString &videoId, bool liked)
{
    rateTrack(videoId, liked ? 1 : 0);
}

void LibraryActions::rateTrack(const QString &videoId, int value)
{
    if (videoId.isEmpty() || value < -1 || value > 1 || !available()
        || m_ratingWrites.contains(videoId))
        return;
    setError({});
    const int previous = rating(videoId, 0);
    const quint64 generation = m_accountGeneration;
    ++m_ratingVersions[videoId];
    m_ratingWrites.insert(videoId);
    Q_EMIT pendingRatingsChanged();
    m_ratings.insert(videoId, value);
    Q_EMIT trackRatingChanged(videoId, value);
    if (value < 0)
        Q_EMIT trackDisliked(videoId);
    const QString endpoint = value > 0 ? QStringLiteral("like/like")
        : value < 0                    ? QStringLiteral("like/dislike")
                                       : QStringLiteral("like/removelike");
    m_endpoints.action(endpoint, videoTarget(videoId),
                       [this, videoId, value, previous, generation](const innertube::Reply &reply) {
        if (generation != m_accountGeneration)
            return;
        m_ratingWrites.remove(videoId);
        Q_EMIT pendingRatingsChanged();
        if (!reply.ok()) {
            m_ratings.insert(videoId, previous);
            Q_EMIT trackRatingChanged(videoId, previous);
            setError(tr("Could not update your rating. Please try again."));
            return;
        }
        Q_EMIT feedback(value > 0       ? tr("Added to liked songs")
                            : value < 0 ? tr("Track disliked")
                                        : tr("Rating removed"));
        Q_EMIT collectionChanged();
    });
}

void LibraryActions::setSaved(const QString &playlistId, bool saved)
{
    if (playlistId.isEmpty() || !available())
        return;
    Q_EMIT savedChanged(playlistId, saved);
    const QJsonObject body {
        {QStringLiteral("target"), QJsonObject {{QStringLiteral("playlistId"), playlistId}}}};
    send(saved ? QStringLiteral("like/like") : QStringLiteral("like/removelike"), body,
         saved ? tr("Could not save that to your library.")
               : tr("Could not remove that from your library."),
         [this, playlistId, saved] { Q_EMIT savedChanged(playlistId, !saved); }, [this, saved] {
        Q_EMIT feedback(saved ? tr("Saved to library") : tr("Removed from library"));
    });
}

void LibraryActions::setSubscribed(const QString &channelId, bool subscribed)
{
    if (channelId.isEmpty() || !available())
        return;
    Q_EMIT subscriptionChanged(channelId, subscribed);
    const QJsonObject body {{QStringLiteral("channelIds"), QJsonArray {channelId}}};
    send(subscribed ? QStringLiteral("subscription/subscribe")
                    : QStringLiteral("subscription/unsubscribe"),
         body,
         subscribed ? tr("Could not follow that artist.") : tr("Could not unfollow that artist."),
         [this, channelId, subscribed] { Q_EMIT subscriptionChanged(channelId, !subscribed); },
         [this, subscribed] {
        Q_EMIT feedback(subscribed ? tr("Following artist") : tr("Artist unfollowed"));
    });
}

void LibraryActions::createPlaylist(const QString &title)
{
    createPlaylistWith(title, {});
}

void LibraryActions::createPlaylistWith(const QString &title, const QStringList &videoIds)
{
    const QString trimmed = title.trimmed();
    if (trimmed.isEmpty() || !available())
        return;
    QJsonObject body {{QStringLiteral("title"), trimmed},
                      {QStringLiteral("privacyStatus"), QStringLiteral("PRIVATE")}};
    if (!videoIds.isEmpty()) {
        QJsonArray seeds;
        for (const QString &videoId : videoIds)
            seeds.append(videoId);
        body.insert(QStringLiteral("videoIds"), seeds);
    }
    send(QStringLiteral("playlist/create"), body, tr("Could not create that playlist."), {},
         [this, trimmed] { Q_EMIT feedback(tr("Created %1").arg(trimmed)); });
}

void LibraryActions::deletePlaylist(const QString &playlistId)
{
    if (playlistId.isEmpty() || !available())
        return;
    const QJsonObject body {{QStringLiteral("playlistId"), withoutListPrefix(playlistId)}};
    send(QStringLiteral("playlist/delete"), body, tr("Could not delete that playlist."), {},
         [this, playlistId] {
        Q_EMIT playlistDeleted(withoutListPrefix(playlistId));
        Q_EMIT feedback(tr("Playlist deleted"));
    });
}

bool LibraryActions::available()
{
    if (!auth::Account::instance().signedIn()) {
        setError(tr("Sign in to change your library."));
        return false;
    }
    if (!net::Connectivity::instance().online()) {
        setError(tr("You're offline. Connect to the internet to change your library."));
        return false;
    }
    return true;
}

void LibraryActions::send(const QString &endpoint, const QJsonObject &body, const QString &failure,
                          const Revert &revert, const Revert &success)
{
    setError({});
    m_endpoints.action(endpoint, body,
                       [this, endpoint, failure, revert, success](const innertube::Reply &reply) {
        if (reply.ok()) {
            if (success)
                success();
            Q_EMIT collectionChanged();
            return;
        }
        qCWarning(logInnerTube) << endpoint << "failed" << reply.error;
        if (revert)
            revert();
        setError(failure);
    });
}

void LibraryActions::togglePinned(const model::Item &item)
{
    const QString key = item.key();
    const bool pinned = !item.pinned;
    const QString token = pinned ? item.actions.pinToken : item.actions.unpinToken;
    if (key.isEmpty() || token.isEmpty() || !available())
        return;
    m_pins.insert(key, pinned);
    Q_EMIT pinChanged(key, pinned);
    send(kFeedbackEndpoint, {{QStringLiteral("feedbackTokens"), QJsonArray {token}}},
         pinned ? tr("Could not pin that to Listen again.")
                : tr("Could not unpin that from Listen again."),
         [this, key, pinned] {
        m_pins.insert(key, !pinned);
        Q_EMIT pinChanged(key, !pinned);
    }, [this, pinned] {
        Q_EMIT feedback(pinned ? tr("Pinned to Listen again") : tr("Unpinned from Listen again"));
    });
}

void LibraryActions::toggleInLibrary(const model::Item &item)
{
    if (!item.savePlaylistId.isEmpty()) {
        setSaved(item.savePlaylistId, !item.saved);
        return;
    }
    const QString key = item.key();
    const bool saved = !item.actions.inLibrary;
    const QString token = saved ? item.actions.libraryAddToken : item.actions.libraryRemoveToken;
    if (key.isEmpty() || token.isEmpty() || !available())
        return;
    m_collected.insert(key, saved);
    Q_EMIT libraryChanged(key, saved);
    send(kFeedbackEndpoint, {{QStringLiteral("feedbackTokens"), QJsonArray {token}}},
         saved ? tr("Could not add that to your library.")
               : tr("Could not remove that from your library."),
         [this, key, saved] {
        m_collected.insert(key, !saved);
        Q_EMIT libraryChanged(key, !saved);
    }, [this, saved] {
        Q_EMIT feedback(saved ? tr("Added to library") : tr("Removed from library"));
    });
}

void LibraryActions::setQueuedForLater(const QString &videoId, bool queued)
{
    if (videoId.isEmpty() || !available())
        return;
    m_later.insert(videoId, queued);
    Q_EMIT laterChanged(videoId, queued);
    const QJsonObject watched {
        {QStringLiteral("action"), QStringLiteral("ACTION_REMOVE_WATCHED_VIDEOS")},
        {QStringLiteral("suppressSuccessToast"), true}};
    const QJsonArray actions = queued
        ? QJsonArray {watched,
                      QJsonObject {
                          {QStringLiteral("action"), QStringLiteral("ACTION_ADD_VIDEO")},
                          {QStringLiteral("addedVideoId"), videoId},
                          {QStringLiteral("dedupeOption"), QStringLiteral("DEDUPE_OPTION_CHECK")}}}
        : QJsonArray {QJsonObject {{QStringLiteral("action"),
                                    QStringLiteral("ACTION_REMOVE_VIDEO_BY_VIDEO_ID")},
                                   {QStringLiteral("removedVideoId"), videoId}},
                      watched};
    edit(kLaterPlaylist, actions,
         queued ? tr("Could not queue that episode for later.")
                : tr("Could not remove that episode from Episodes for Later."),
         [this, videoId, queued] {
        m_later.insert(videoId, !queued);
        Q_EMIT laterChanged(videoId, !queued);
    }, [this, queued] {
        Q_EMIT feedback(queued ? tr("Queued to Episodes for Later")
                               : tr("Removed from Episodes for Later"));
    });
}

void LibraryActions::setPlayed(const model::Item &item, bool played)
{
    const QString videoId = item.track.videoId;
    const QString token = played ? item.actions.playedToken : item.actions.unplayedToken;
    if (videoId.isEmpty() || token.isEmpty() || !available())
        return;
    m_played.insert(videoId, played);
    Q_EMIT playedChanged(videoId, played);
    send(kFeedbackEndpoint, {{QStringLiteral("feedbackTokens"), QJsonArray {token}}},
         played ? tr("Could not mark that episode as played.")
                : tr("Could not mark that episode as unplayed."),
         [this, videoId, played] {
        m_played.insert(videoId, !played);
        Q_EMIT playedChanged(videoId, !played);
    }, [this, played] {
        Q_EMIT feedback(played ? tr("Episode marked as played") : tr("Episode marked as unplayed"));
    });
}

void LibraryActions::addToPlaylist(const QString &playlistId, const QString &playlistTitle,
                                   const QStringList &videoIds)
{
    if (playlistId.isEmpty() || videoIds.isEmpty() || !available())
        return;
    if (playlistId == QLatin1String("LM")) {
        for (const QString &videoId : videoIds)
            rateTrack(videoId, 1);
        return;
    }
    QJsonArray actions;
    for (const QString &videoId : videoIds) {
        actions.append(QJsonObject {{QStringLiteral("action"), QStringLiteral("ACTION_ADD_VIDEO")},
                                    {QStringLiteral("addedVideoId"), videoId}});
    }
    const int count = videoIds.size();
    edit(playlistId, actions, tr("Could not add that to the playlist."), {},
         [this, playlistTitle, count] {
        Q_EMIT feedback(tr("Added %n songs to %1", nullptr, count).arg(playlistTitle));
    });
}

void LibraryActions::removeFromPlaylist(const QString &playlistId, const QStringList &videoIds,
                                        const QStringList &setVideoIds)
{
    if (playlistId.isEmpty() || videoIds.size() != setVideoIds.size() || videoIds.isEmpty()
        || !available())
        return;
    QJsonArray actions;
    for (int i = 0; i < videoIds.size(); ++i) {
        if (videoIds.at(i).isEmpty() || setVideoIds.at(i).isEmpty()) {
            setError(tr("Reload the playlist before removing this track."));
            return;
        }
        actions.append(
            QJsonObject {{QStringLiteral("action"), QStringLiteral("ACTION_REMOVE_VIDEO")},
                         {QStringLiteral("removedVideoId"), videoIds.at(i)},
                         {QStringLiteral("setVideoId"), setVideoIds.at(i)}});
    }
    const QString target = withoutListPrefix(playlistId);
    const int count = videoIds.size();
    edit(playlistId, actions, tr("Could not remove that from the playlist."),
         [this, target] { Q_EMIT playlistReordered(target); }, [this, target, setVideoIds, count] {
        Q_EMIT playlistItemsRemoved(target, setVideoIds);
        Q_EMIT feedback(tr("Removed %n songs from the playlist", nullptr, count));
    });
}

void LibraryActions::editPlaylist(const QString &playlistId, const QString &name,
                                  const QString &description, const QString &privacy)
{
    if (playlistId.isEmpty() || !available())
        return;
    QJsonArray actions;
    if (!name.isEmpty()) {
        actions.append(
            QJsonObject {{QStringLiteral("action"), QStringLiteral("ACTION_SET_PLAYLIST_NAME")},
                         {QStringLiteral("playlistName"), name}});
    }
    actions.append(
        QJsonObject {{QStringLiteral("action"), QStringLiteral("ACTION_SET_PLAYLIST_DESCRIPTION")},
                     {QStringLiteral("playlistDescription"), description}});
    if (!privacy.isEmpty()) {
        actions.append(
            QJsonObject {{QStringLiteral("action"), QStringLiteral("ACTION_SET_PLAYLIST_PRIVACY")},
                         {QStringLiteral("playlistPrivacy"), privacy}});
    }
    const QString target = withoutListPrefix(playlistId);
    edit(playlistId, actions, tr("Could not save those playlist details."), {},
         [this, target, name, description, privacy] {
        Q_EMIT playlistEdited(target, name, description, privacy);
        Q_EMIT feedback(tr("Playlist updated"));
    });
}

void LibraryActions::movePlaylistItem(const QString &playlistId, const QString &setVideoId,
                                      const QString &predecessor,
                                      const std::function<void(bool)> &handler)
{
    if (playlistId.isEmpty() || setVideoId.isEmpty() || setVideoId == predecessor || !available()) {
        handler(false);
        return;
    }
    QJsonObject action {{QStringLiteral("action"), QStringLiteral("ACTION_MOVE_VIDEO_AFTER")},
                        {QStringLiteral("setVideoId"), setVideoId}};
    if (!predecessor.isEmpty())
        action.insert(QStringLiteral("movedSetVideoIdPredecessor"), predecessor);
    edit(playlistId, QJsonArray {action},
         QStringLiteral("Could not move that track. Its order was restored."),
         [handler] { handler(false); }, [handler] { handler(true); });
}

void LibraryActions::setPlaylistOrder(const QString &playlistId, const model::SortOption &option,
                                      const std::function<void(bool)> &handler)
{
    if (playlistId.isEmpty() || !option.stored() || !available()) {
        handler(false);
        return;
    }
    const QJsonObject action = option.videoOrder >= 0
        ? QJsonObject {{QStringLiteral("action"),
                        QStringLiteral("ACTION_SET_PLAYLIST_VIDEO_ORDER")},
                       {QStringLiteral("playlistVideoOrder"), option.videoOrder}}
        : QJsonObject {{QStringLiteral("action"),
                        QStringLiteral("ACTION_SET_PLAYLIST_DYNAMIC_SORT_PREFERENCE")},
                       {QStringLiteral("playlistDynamicSortPreference"), option.dynamicSort}};
    const QString target = withoutListPrefix(playlistId);
    edit(playlistId, QJsonArray {action}, QStringLiteral("Could not reorder that playlist."),
         [this, target, handler] {
        handler(false);
        Q_EMIT playlistReordered(target);
    }, [this, target, handler] {
        handler(true);
        Q_EMIT playlistReordered(target);
    });
}

void LibraryActions::edit(const QString &playlistId, const QJsonArray &actions,
                          const QString &failure, const Revert &revert, const Revert &success)
{
    setError({});
    const QJsonObject body {{QStringLiteral("playlistId"), withoutListPrefix(playlistId)},
                            {QStringLiteral("actions"), actions}};
    m_endpoints.action(kEditEndpoint, body,
                       [this, failure, revert, success](const innertube::Reply &reply) {
        const QString status = reply.json.value(QStringLiteral("status")).toString();
        if (reply.ok() && (status.isEmpty() || status == kEditSucceeded)) {
            if (success)
                success();
            Q_EMIT collectionChanged();
            return;
        }
        qCWarning(logInnerTube) << "edit_playlist failed" << reply.error << status;
        if (revert)
            revert();
        const int duplicates = duplicatesRejected(reply.json);
        setError(duplicates > 0 ? tr("%n songs are already in this playlist", nullptr, duplicates)
                                : failure);
    });
}

void LibraryActions::setError(const QString &error)
{
    if (m_error == error)
        return;
    m_error = error;
    Q_EMIT errorChanged();
}

}
