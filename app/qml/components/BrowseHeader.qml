import QtQuick
import HtMusic

Column {
    id: root

    required property BrowseModel page

    property bool podcastInline: false

    readonly property bool detail: !!root.page && root.page.detail && Browser.section !== 1
    readonly property bool podcast: !!root.page && root.page.kind === "podcast"
    readonly property bool episode: !!root.page && root.page.kind === "episode"
    readonly property bool downloads: !!root.page && root.page.kind === "downloads"
    readonly property bool history: !!root.page && root.page.kind === "history"
    readonly property bool listing: !!root.page && root.page.kind !== "profile" && !root.episode
    readonly property bool sortable: !!root.page && root.page.sortable
    readonly property bool searchable: !!root.page && root.page.searchable && !root.downloads
    readonly property bool searching: !!root.page && root.page.kind === "search" && !root.page.local
    readonly property bool scoped: root.searching && root.page.scopes.length > 1
    readonly property bool historyFiltered: root.history && root.page.filterable
    readonly property bool filterable: root.podcast && root.page.chips.length > 0
    readonly property bool filtersShown: root.sortable || root.filterable
    readonly property real finderWidth: 260
    readonly property real filtersWidth: (root.sortable ? sorter.implicitWidth : 0)
        + (root.filterable ? chipFilters.naturalWidth : 0)
        + (root.sortable && root.filterable ? filters.spacing : 0)
    readonly property bool finderBeside: !root.searchable || !root.filtersShown
        || root.width >= root.filtersWidth + 16 + root.finderWidth
    readonly property real downloadedBytes: Downloads.smartBytes + Downloads.forcedBytes

    signal menuRequested(var request)
    signal filterDismissed

    spacing: 18

    PodcastHeader {
        width: parent.width
        visible: root.podcast && root.podcastInline
        page: root.podcast && root.podcastInline ? root.page : null
        inline: true
        onMenuRequested: request => root.menuRequested(request)
    }

    Item {
        width: parent.width
        height: !visible ? 0 : root.page && root.page.artId.length > 0 ? 192
            : Math.max(titles.implicitHeight, downloadFinder.visible ? downloadFinder.height : 0) + 16
        visible: !root.podcast

        Artwork {
            id: cover

            anchors.left: parent.left
            width: visible ? 176 : 0
            height: width
            visible: root.page && root.page.artId.length > 0
            artId: root.page ? root.page.artId : ""
            rounding: root.page && root.page.entry.circular ? Theme.rounding.full : Theme.rounding.normal
        }

        RevealingSelect {
            id: trailingSelect

            anchors.right: parent.right
            anchors.verticalCenter: titles.verticalCenter
            width: implicitWidth
            shown: root.scoped || root.historyFiltered
            choices: root.scoped ? root.page.scopes : root.historyFiltered ? root.page.chips : []
            current: root.scoped ? root.page.scopeIndex : root.historyFiltered ? root.page.filterIndex : -1
            heading: root.searching ? qsTr("Search in") : ""
            interactive: !!root.page && !root.page.loading
            onPicked: index => {
                if (root.searching)
                    Browser.chooseScope(index);
                else
                    Browser.chooseChip(index);
            }
        }

        SearchBar {
            id: downloadFinder

            anchors.right: parent.right
            anchors.verticalCenter: titles.verticalCenter
            width: Math.min(300, parent.width * 0.45)
            visible: root.downloads
            enabled: Downloads.ids.length > 0 || text.length > 0
            opacity: enabled ? 1 : 0.5
            icon: "search"
            placeholder: qsTr("Search downloads")
            onTextChanged: if (root.downloads) root.page.filter = text.trim()
            onEditingFinished: root.filterDismissed()
        }

        Column {
            id: titles

            anchors.left: cover.right
            anchors.leftMargin: cover.visible ? 24 : 0
            anchors.right: downloadFinder.visible ? downloadFinder.left
                : trailingSelect.visible ? trailingSelect.left : parent.right
            anchors.rightMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            spacing: 14

            SectionHeader {
                width: parent.width
                title: !root.page ? ""
                    : root.page.kind === "search" ? qsTr("Search: %1").arg(root.page.title)
                    : root.downloads ? qsTr("Downloads")
                    : root.history ? qsTr("History")
                    : root.page.title
                subtitle: !root.page ? ""
                    : root.downloads ? (Downloads.ids.length === 0 ? ""
                        : root.downloadedBytes >= 1073741824
                        ? qsTr("%n downloads - %1 GB", "", Downloads.ids.length).arg((root.downloadedBytes / 1073741824).toFixed(1))
                        : qsTr("%n downloads - %1 MB", "", Downloads.ids.length).arg((root.downloadedBytes / 1048576).toFixed(1)))
                    : (root.page.subtitle || root.page.byline)
            }

            StyledText {
                width: parent.width
                visible: !!root.page && root.page.description.length > 0
                text: root.page ? root.page.description : ""
                font.pixelSize: Theme.font.smallie
                color: Theme.colOnSurfaceVariant
                wrapMode: Text.WordWrap
                maximumLineCount: 2
                elide: Text.ElideRight
            }

            Row {
                spacing: 8

                PillButton {
                    visible: root.page && root.page.playable && root.detail && root.listing
                    text: qsTr("Play")
                    icon: "play_arrow"
                    toggled: true
                    onClicked: Browser.playPage()
                }

                PillButton {
                    visible: root.page && root.page.playable && root.detail && root.listing
                    text: qsTr("Shuffle")
                    icon: "shuffle"
                    onClicked: Browser.playShuffled(root.page.entry)
                }

                PillButton {
                    visible: root.episode && root.page.entry.track.valid
                    text: root.page && root.page.entry.progress > 0 && !root.page.entry.played
                        ? qsTr("Resume") : qsTr("Play")
                    icon: "play_arrow"
                    toggled: true
                    onClicked: Browser.playEntry(root.page.entry)
                }

                PillButton {
                    visible: root.episode && Account.signedIn && root.page.entry.actions.laterable
                    interactive: Connectivity.online
                    opacity: interactive ? 1 : 0.4
                    text: root.page && root.page.entry.actions.later ? qsTr("Queued for later")
                        : qsTr("Save for later")
                    icon: root.page && root.page.entry.actions.later ? "playlist_add_check" : "playlist_add"
                    toggled: root.page && root.page.entry.actions.later
                    onClicked: LibraryActions.setQueuedForLater(root.page.entry.track.videoId,
                                                                !root.page.entry.actions.later)
                }

                PillButton {
                    visible: root.page && root.page.detail && Account.signedIn && root.page.collectable
                    interactive: Connectivity.online
                    opacity: interactive ? 1 : 0.4
                    text: root.page && root.page.saved ? qsTr("Saved") : qsTr("Save")
                    icon: root.page && root.page.saved ? "library_add_check" : "library_add"
                    toggled: root.page && root.page.saved
                    onClicked: root.page.toggleSaved()
                }

                PillButton {
                    visible: root.page && root.page.detail && Account.signedIn && root.page.followable
                    interactive: Connectivity.online
                    opacity: interactive ? 1 : 0.4
                    text: root.page && root.page.subscribed
                        ? (root.page.kind === "profile" ? qsTr("Subscribed") : qsTr("Following"))
                        : (root.page.kind === "profile" ? qsTr("Subscribe") : qsTr("Follow"))
                    icon: root.page && root.page.subscribed ? "person_check" : "person_add"
                    toggled: root.page && root.page.subscribed
                    onClicked: root.page.toggleSubscribed()
                }

                IconButton {
                    id: shareButton

                    anchors.verticalCenter: parent.verticalCenter
                    visible: root.detail
                    icon: "share"
                    Accessible.name: qsTr("Share")
                    onClicked: Actions.share(root.page.entry)
                }

                IconButton {
                    id: overflow

                    anchors.verticalCenter: parent.verticalCenter
                    visible: root.detail
                    icon: "more_vert"
                    Accessible.name: qsTr("More actions")
                    onClicked: {
                        const spot = root.mapFromItem(overflow, 0, overflow.height);
                        root.menuRequested({ "entry": root.page.entry, "index": -1, "model": null,
                            "source": root, "x": spot.x, "y": spot.y, "selectable": false });
                    }
                }
            }
        }
    }

    Row {
        visible: !!root.page && root.page.kind === "search" && root.page.local
        spacing: 10

        Sym {
            anchors.verticalCenter: parent.verticalCenter
            text: "download_for_offline"
            iconSize: Theme.font.larger
            color: Theme.colOnSurfaceVariant
        }

        StyledText {
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("You're offline, so this only searches your downloads")
            font.pixelSize: Theme.font.smallie
            color: Theme.colOnSurfaceVariant
        }
    }

    ChipRow {
        width: parent.width
        visible: root.searching && chips.length > 0
        clearable: false
        selectedIcon: ""
        chips: root.searching ? root.page.chips : []
        interactive: !!root.page && !root.page.loading
        onChipClicked: index => Browser.chooseChip(index)
    }

    Item {
        width: parent.width
        height: !visible ? 0 : root.finderBeside ? Math.max(46, filters.height)
            : (filters.visible ? filters.height + 12 : 0) + finder.height
        visible: root.sortable || root.searchable || root.filterable

        Row {
            id: filters

            anchors.left: parent.left
            anchors.right: finder.visible && root.finderBeside ? finder.left : parent.right
            anchors.rightMargin: finder.visible && root.finderBeside ? 16 : 0
            y: root.finderBeside ? (parent.height - height) / 2 : 0
            visible: root.filtersShown
            spacing: 10

            BrowseFilter {
                id: sorter

                anchors.verticalCenter: parent.verticalCenter
                visible: root.sortable
                page: root.page
            }

            ChipRow {
                id: chipFilters

                anchors.verticalCenter: parent.verticalCenter
                width: filters.width - (sorter.visible ? sorter.width + filters.spacing : 0)
                visible: root.filterable
                clearable: false
                chips: root.filterable ? root.page.chips : []
                interactive: !!root.page && !root.page.loading
                onChipClicked: index => Browser.chooseChip(index)
            }
        }

        SearchBar {
            id: finder

            x: root.finderBeside && root.filtersShown ? parent.width - width : 0
            y: root.finderBeside ? (parent.height - height) / 2 : filters.visible ? filters.height + 12 : 0
            width: root.finderBeside ? root.finderWidth : Math.min(parent.width, 420)
            visible: root.searchable
            icon: "search"
            placeholder: root.podcast ? qsTr("Find in show") : qsTr("Find in playlist")
            onTextChanged: if (root.searchable) root.page.filter = text.trim()
            onEditingFinished: root.filterDismissed()
        }
    }

    Item {
        width: parent.width
        height: 8
    }

    onPageChanged: {
        const filter = root.page ? root.page.filter : "";
        finder.text = root.searchable ? filter : "";
        downloadFinder.text = root.downloads ? filter : "";
    }
}
