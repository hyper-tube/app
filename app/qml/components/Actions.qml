pragma Singleton

import QtQuick
import HtMusic

QtObject {
    id: root

    readonly property var privacyLabels: ({
        "PUBLIC": qsTr("Public"),
        "UNLISTED": qsTr("Unlisted"),
        "PRIVATE": qsTr("Private")
    })

    signal editRequested(var entry)
    signal deleteRequested(var entry)
    signal selectRequested(var entry)

    function entryTracks(entry) {
        return entry.playable ? [entry.track] : [];
    }

    function downloadState(entry) {
        return entry.playable ? Downloads.stateOf(entry.track.videoId) : Downloads.Absent;
    }

    function downloadAction(entry) {
        switch (root.downloadState(entry)) {
        case Downloads.Ready:
            return { "icon": "download_done", "text": qsTr("Remove download"), "action": "undownload", "active": true };
        case Downloads.Waiting:
        case Downloads.Fetching:
            return { "icon": "downloading", "text": qsTr("Cancel download"), "action": "undownload" };
        case Downloads.Failed:
            return { "icon": "download", "text": qsTr("Retry download"), "action": "download" };
        default:
            return { "icon": "download", "text": qsTr("Download"), "action": "download" };
        }
    }

    function share(entry) {
        const url = entry.shareUrl;
        if (url.length === 0) {
            Toasts.show(qsTr("There is no link to share for this."), true);
            return;
        }
        if (Clipboard.copy(url))
            Toasts.show(qsTr("Link copied to clipboard"));
        else
            Toasts.show(qsTr("Could not copy that link."), true);
    }

    function laterAction(entry) {
        return entry.actions.later
            ? { icon: "playlist_add_check", text: qsTr("Remove from Episodes for Later"), action: "unlater", active: true }
            : { icon: "playlist_add", text: qsTr("Queue to Episodes for Later"), action: "later" };
    }

    function playedAction(entry) {
        return entry.played
            ? { icon: "undo", text: qsTr("Mark as unplayed"), action: "unplayed" }
            : { icon: "check_circle", text: qsTr("Mark as played"), action: "played" };
    }

    function destinations(menu, entry, online) {
        if (!online)
            return;
        if (entry.actions.episodeId.length > 0)
            menu.push({ icon: "info", text: qsTr("Go to episode"), action: "episode" });
        if (entry.actions.podcastId.length > 0)
            menu.push({ icon: "podcasts", text: qsTr("Go to podcast"), action: "podcast" });
        if (entry.actions.profileId.length > 0 && entry.kind !== "profile")
            menu.push({ icon: "account_circle", text: qsTr("Go to profile"), action: "profile" });
    }

    function build(entry, options) {
        const menu = [];
        const song = entry.playable;
        const episode = entry.episode;
        const artist = entry.kind === "artist" || entry.kind === "profile";
        const chosen = options.selectionCount > 0;
        const online = Connectivity.online;
        const writable = Account.signedIn && online;
        if (options.selectable && !chosen)
            menu.push({ icon: "check_circle", text: qsTr("Select"), action: "select" });
        if (writable && episode && entry.actions.laterable)
            menu.push(root.laterAction(entry));
        if (writable && episode && entry.actions.markable)
            menu.push(root.playedAction(entry));
        if (online && entry.actions.mixable)
            menu.push({ icon: "radio", text: qsTr("Start mix"), action: "mix" });
        if (!artist && (online || song)) {
            menu.push({ icon: "playlist_play", text: qsTr("Play next"), action: "next" });
            menu.push({ icon: "queue_music", text: qsTr("Add to queue"), action: "queue" });
        }
        if (writable && (song || entry.kind === "album" || entry.kind === "playlist"))
            menu.push({ icon: "playlist_add", text: qsTr("Save to playlist"), action: "save" });
        if (writable && song && !episode) {
            menu.push(entry.track.liked
                ? { icon: "favorite", text: qsTr("Remove from liked songs"), action: "unlike", active: true }
                : { icon: "favorite_border", text: qsTr("Add to liked songs"), action: "like" });
        }
        if (writable && (entry.actions.collectable || entry.savePlaylistId.length > 0)) {
            const held = entry.savePlaylistId.length > 0 ? entry.saved : entry.actions.inLibrary;
            menu.push(held
                ? { icon: "library_add_check", text: qsTr("Remove from library"), action: "uncollect", active: true }
                : { icon: "library_add", text: qsTr("Add to library"), action: "collect" });
        }
        if (online && entry.actions.artistId.length > 0)
            menu.push({ icon: "person", text: qsTr("Go to artist page"), action: "artist" });
        if (online && entry.actions.albumId.length > 0)
            menu.push({ icon: "album", text: qsTr("Go to album"), action: "album" });
        root.destinations(menu, entry, online);
        if (writable && artist && entry.channelId.length > 0) {
            menu.push(entry.subscribed
                ? { icon: "person_check", text: qsTr("Unsubscribe"), action: "unsubscribe", active: true }
                : { icon: "person_add", text: qsTr("Subscribe"), action: "subscribe" });
        }
        if (writable && entry.actions.editable)
            menu.push({ icon: "edit", text: qsTr("Edit playlist"), action: "edit" });
        if (!artist && !entry.track.live) {
            if (song && root.downloadState(entry) === Downloads.Ready && !Downloads.isForced(entry.track.videoId))
                menu.push({ icon: "keep", text: qsTr("Keep download"), action: "download" });
            if (online || song)
                menu.push(root.downloadAction(entry));
        }
        menu.push({ icon: "share", text: qsTr("Share"), action: "share" });
        if (writable && entry.actions.pinnable) {
            menu.push(entry.pinned
                ? { icon: "keep_off", text: qsTr("Unpin from Listen again"), action: "unpin", active: true }
                : { icon: "keep", text: qsTr("Pin to Listen again"), action: "pin" });
        }
        if (writable && entry.actions.removable)
            menu.push({ icon: "playlist_remove", text: qsTr("Remove from playlist"), action: "remove", destructive: true });
        if (writable && entry.deletable && entry.playlistTarget.length > 0)
            menu.push({ icon: "delete", text: qsTr("Delete playlist"), action: "delete", destructive: true });
        return menu;
    }

    function buildPlayer(entry) {
        const menu = [];
        if (!entry.playable)
            return menu;
        const online = Connectivity.online;
        const writable = Account.signedIn && online;
        const station = entry.track.live;
        const episode = entry.episode;
        if (writable && episode && entry.actions.laterable)
            menu.push(root.laterAction(entry));
        if (online && !station && entry.actions.mixable)
            menu.push({ icon: "radio", text: qsTr("Start mix"), action: "mix" });
        menu.push({ icon: "playlist_play", text: qsTr("Play next"), action: "repeatNext" });
        menu.push({ icon: "queue_music", text: qsTr("Add to queue"), action: "repeatLater" });
        if (writable && !episode) {
            menu.push(entry.track.liked
                ? { icon: "favorite", text: qsTr("Remove from liked songs"), action: "unlike", active: true }
                : { icon: "favorite_border", text: qsTr("Add to liked songs"), action: "like" });
        }
        if (!station && (online || root.downloadState(entry) === Downloads.Ready))
            menu.push(root.downloadAction(entry));
        if (writable)
            menu.push({ icon: "playlist_add", text: qsTr("Save to playlist"), action: "save" });
        menu.push({ icon: "playlist_remove", text: qsTr("Remove from queue"), action: "dequeueCurrent" });
        if (episode)
            root.destinations(menu, entry, online);
        menu.push({ icon: "share", text: qsTr("Share"), action: "share" });
        if (writable && entry.actions.pinnable) {
            menu.push(entry.pinned
                ? { icon: "keep_off", text: qsTr("Unpin from Listen again"), action: "unpin", active: true }
                : { icon: "keep", text: qsTr("Pin to Listen again"), action: "pin" });
        }
        menu.push({ icon: "clear_all", text: qsTr("Dismiss queue"), action: "dismissQueue", destructive: true });
        return menu;
    }

    function run(entry, action) {
        switch (action) {
        case "select":
            root.selectRequested(entry);
            break;
        case "mix":
            Browser.playMix(entry);
            break;
        case "next":
            Browser.resolveTracks(entry, "next");
            break;
        case "queue":
            Browser.resolveTracks(entry, "queue");
            break;
        case "save":
            if (entry.playable)
                PlaylistTargets.show([entry.track.videoId]);
            else
                Browser.resolveTracks(entry, "save");
            break;
        case "like":
            LibraryActions.rateTrack(entry.track.videoId, 1);
            break;
        case "unlike":
            LibraryActions.rateTrack(entry.track.videoId, 0);
            break;
        case "collect":
        case "uncollect":
            LibraryActions.toggleInLibrary(entry);
            break;
        case "artist":
            Browser.openPage(entry.actions.artistId, "artist");
            break;
        case "album":
            Browser.openPage(entry.actions.albumId, "album");
            break;
        case "episode":
            Browser.openPage(entry.actions.episodeId, "episode");
            break;
        case "podcast":
            Browser.openPage(entry.actions.podcastId, "podcast");
            break;
        case "profile":
            Browser.openPage(entry.actions.profileId, "profile");
            break;
        case "later":
        case "unlater":
            LibraryActions.setQueuedForLater(entry.track.videoId, action === "later");
            break;
        case "played":
        case "unplayed":
            LibraryActions.setPlayed(entry, action === "played");
            break;
        case "subscribe":
            LibraryActions.setSubscribed(entry.channelId, true);
            break;
        case "unsubscribe":
            LibraryActions.setSubscribed(entry.channelId, false);
            break;
        case "edit":
            root.editRequested(entry);
            break;
        case "download":
            if (entry.playable)
                Downloads.keep([entry.track]);
            else
                Browser.resolveTracks(entry, "download");
            break;
        case "undownload":
            Downloads.discard(root.entryTracks(entry));
            break;
        case "share":
            root.share(entry);
            break;
        case "pin":
        case "unpin":
            LibraryActions.togglePinned(entry);
            break;
        case "remove":
            LibraryActions.removeFromPlaylist(entry.actions.sourcePlaylistId,
                [entry.track.videoId], [entry.actions.setVideoId]);
            break;
        case "delete":
            root.deleteRequested(entry);
            break;
        case "repeatNext":
            PlaybackController.repeatCurrent(true);
            Toasts.show(qsTr("Playing next"));
            break;
        case "repeatLater":
            PlaybackController.repeatCurrent(false);
            Toasts.show(qsTr("Added to queue"));
            break;
        case "dequeueCurrent":
            PlaybackController.removeFromQueue(PlaybackController.queueIndex);
            break;
        case "dismissQueue":
            PlaybackController.clearQueue();
            break;
        }
    }
}
