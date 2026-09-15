import QtQuick
import HtMusic

Item {
    id: root

    property real reveal: 1
    property real contentWidth: width
    property bool selectMode: false
    property var selection: ({})
    property PlaylistDrag reorder: null
    property real removal: 0

    readonly property bool dragged: !!root.reorder && root.reorder.active
        && root.sectionIndex === 0 && root.rowType === "song" && root.itemIndex === root.reorder.sourceIndex
    readonly property real dragOffset: root.reorder && root.sectionIndex === 0 && root.rowType === "song"
        ? root.reorder.offset(root.itemIndex) : 0

    required property string rowType
    required property int sectionIndex
    required property int itemIndex
    required property var entry
    required property ItemModel entries
    required property var cells
    required property string sectionTitle
    required property string sectionSubtitle
    required property var moreDestination
    required property bool moreAvailable
    required property bool busy
    required property var lead
    required property string sectionDescription
    required property string sectionMessage
    required property BrowseModel page
    required property int columns
    required property ListView viewport

    readonly property bool ghostMore: !!root.page && root.page.kind === "library"
    readonly property bool videoCovers: !!root.page && root.page.kind === "feed"
    readonly property bool canSeeAll: !!moreDestination
        && (moreDestination.browseId.length > 0 || moreDestination.kind === "search")
    readonly property bool selectable: !!root.page && root.page.kind === "playlist"
        && root.sectionIndex === 0
    readonly property bool nearViewport: y < viewport.contentY + viewport.height + 500
        && y + height > viewport.contentY - 100

    readonly property real usableWidth: root.contentWidth - Theme.size.bleed * 2

    signal menuRequested(var request)
    signal selectionToggled(ItemModel model, int index)

    height: loader.height
    opacity: 1 - root.removal
    scale: 1 - 0.035 * root.removal
    enabled: root.removal === 0

    transform: Translate {
        y: root.dragOffset
        Behavior on y {
            enabled: !!root.reorder && root.reorder.dragging
            NumberAnimation {
                duration: Theme.duration.spatialFast
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveFastSpatial
            }
        }
    }

    Loader {
        id: loader

        x: (root.width - root.contentWidth) / 2 + Theme.size.bleed
        width: root.usableWidth
        opacity: root.dragged ? 0 : root.reveal
        scale: 0.98 + 0.02 * root.reveal
        transformOrigin: Item.TopLeft
        transform: Translate { y: 28 * (1 - root.reveal) }
        enabled: root.reveal === 1
        sourceComponent: root.rowType === "song" ? song : root.rowType === "grid" ? grid
            : root.rowType === "episode" ? episode : root.rowType === "card" ? card
            : root.rowType === "ranked" ? ranked
            : root.rowType === "heading" ? heading : root.rowType === "shelf" ? shelf
            : root.rowType === "moods" ? moods : root.rowType === "destinations" ? destinations
            : root.rowType === "message" ? message : root.rowType === "description" ? description
            : root.rowType === "chips" ? chips : continuation
    }

    Component {
        id: song

        SongRow {
            id: row

            width: root.usableWidth
            position: root.itemIndex + 1
            numbered: root.sectionIndex === 0 && root.page
                && (root.page.kind === "playlist" || root.page.kind === "album")
            title: root.entry.title
            artist: root.entry.episode ? root.entry.subtitle : root.entry.track.artist || root.entry.subtitle
            album: root.entry.playable ? root.entry.track.album : ""
            artId: root.entry.artId
            durationMs: root.entry.track.durationMs
            live: root.entry.track.live
            videoId: root.entry.track.videoId
            liked: root.entry.track.liked
            downloaded: root.entry.playable && Downloads.ids.includes(root.entry.track.videoId)
            playable: root.entry.playable
            circular: root.entry.circular
            reorderable: !!root.reorder && !!root.page && root.page.reorderable
                && root.selectable && !root.selectMode && root.entry.actions.removable
            reorderEnabled: !root.page || !root.page.reordering
            onDragStarted: (x, y) => root.reorder.begin(root, row, x, y)
            onDragMoved: (x, y) => root.reorder.update(row, x, y)
            onDragFinished: cancelled => root.reorder.finish(cancelled)
            selectMode: root.selectMode && root.selectable
            selected: root.selection[root.itemIndex] === true
            active: root.entry.playable && PlaybackController.track.videoId === root.entry.track.videoId
            onPlayRequested: Browser.activate(root.entries, root.itemIndex)
            onSelectToggled: root.selectionToggled(root.entries, root.itemIndex)
            onMenuRequested: (x, y) => root.menuRequested({
                "entry": root.entry, "index": root.itemIndex, "model": root.entries,
                "source": row, "x": x, "y": y, "selectable": root.selectable })
        }
    }

    Component {
        id: episode

        EpisodeRow {
            id: episodeRow

            width: root.usableWidth
            entry: root.entry
            active: root.entry.playable && PlaybackController.track.videoId === root.entry.track.videoId
            downloaded: root.entry.playable && Downloads.ids.includes(root.entry.track.videoId)
            onPlayRequested: Browser.activate(root.entries, root.itemIndex)
            onMenuRequested: (x, y) => root.menuRequested({
                "entry": root.entry, "index": root.itemIndex, "model": root.entries,
                "source": episodeRow, "x": x, "y": y, "selectable": false })
        }
    }

    Component {
        id: card

        TopResultCard {
            width: root.usableWidth
            lead: root.lead
            cells: root.cells
            entries: root.entries
            onMenuRequested: request => root.menuRequested(request)
        }
    }

    Component {
        id: message

        Item {
            id: messageRow

            readonly property bool clearable: !!root.page && root.page.chipSelected

            width: root.usableWidth
            implicitHeight: notice.implicitHeight + 48

            Column {
                id: notice

                anchors.centerIn: parent
                spacing: 18

                EmptyState {
                    anchors.horizontalCenter: parent.horizontalCenter
                    icon: messageRow.clearable ? "filter_list_off" : "search_off"
                    title: root.sectionMessage
                }

                PillButton {
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: messageRow.clearable
                    interactive: !root.page.loading
                    text: qsTr("Clear filters")
                    icon: "filter_list_off"
                    toggled: true
                    onClicked: root.page.clearChip()
                }
            }
        }
    }

    Component {
        id: description

        StyledText {
            width: root.usableWidth
            bottomPadding: 16
            text: root.sectionDescription
            textFormat: Text.PlainText
            font.pixelSize: Theme.font.small
            color: Theme.colOnSurfaceVariant
            wrapMode: Text.WordWrap
            lineHeight: 1.2
        }
    }

    Component {
        id: heading

        Item {
            readonly property bool compact: !!root.page && root.page.kind === "search"

            width: root.usableWidth
            implicitHeight: header.visible ? header.implicitHeight + (compact ? 18 : 28) : 0

            SectionHeader {
                id: header

                width: parent.width
                y: 12
                visible: title.length > 0 || subtitle.length > 0 || root.canSeeAll
                title: root.page && root.sectionTitle === root.page.title ? "" : root.sectionTitle
                subtitle: root.sectionSubtitle
                moreAvailable: root.canSeeAll
                moreGhost: root.ghostMore
                onMoreRequested: Browser.open(root.moreDestination)
            }
        }
    }

    Component {
        id: grid

        Row {
            width: root.usableWidth
            height: Math.min(200, root.usableWidth / root.columns - 24) + 88
            spacing: 4

            Repeater {
                model: root.cells

                delegate: Card {
                    id: card

                    required property var modelData
                    required property int index

                    artSize: Math.min(200, root.usableWidth / root.columns - 24)
                    title: modelData.title
                    subtitle: modelData.subtitle
                    artId: modelData.artId
                    circular: modelData.circular
                    playable: modelData.playable
                    pinned: modelData.pinned
                    live: modelData.track.live
                    active: modelData.track.valid
                        && PlaybackController.track.videoId === modelData.track.videoId
                    onClicked: Browser.activate(root.entries, root.itemIndex + card.index)
                    onPlayRequested: Browser.playItem(root.entries, root.itemIndex + card.index)
                    onMenuRequested: (x, y) => root.menuRequested({
                        "entry": card.modelData, "index": root.itemIndex + card.index,
                        "model": root.entries, "source": card, "x": x, "y": y, "selectable": false })
                }
            }
        }
    }

    Component {
        id: shelf

        Shelf {
            width: root.usableWidth
            title: root.sectionTitle
            subtitle: root.sectionSubtitle
            cards: root.entries
            videoCovers: root.videoCovers
            moreAvailable: root.canSeeAll
            moreGhost: root.ghostMore
            onMoreRequested: Browser.open(root.moreDestination)
            onCardActivated: index => Browser.activate(root.entries, index)
            onCardPlayRequested: index => Browser.playItem(root.entries, index)
            onCardMenuRequested: (index, source, x, y) => root.menuRequested({
                "entry": root.entries.get(index), "index": index, "model": root.entries,
                "source": source, "x": x, "y": y, "selectable": false })
            onEndReached: {
                if (root.moreAvailable && !root.busy && root.page.error.length === 0)
                    root.page.loadMore(root.sectionIndex);
            }
        }
    }

    Component {
        id: ranked

        RankedShelf {
            width: root.usableWidth
            title: root.sectionTitle
            subtitle: root.sectionSubtitle
            cells: root.cells
            moreAvailable: root.canSeeAll
            onMoreRequested: Browser.open(root.moreDestination)
            onActivated: index => Browser.activate(root.entries, index)
            onMenuRequested: (index, source, x, y) => root.menuRequested({
                "entry": root.entries.get(index), "index": index, "model": root.entries,
                "source": source, "x": x, "y": y, "selectable": false })
            onEndReached: {
                if (root.moreAvailable && !root.busy && root.page.error.length === 0)
                    root.page.loadMore(root.sectionIndex);
            }
        }
    }

    Component {
        id: moods

        MoodShelf {
            width: root.usableWidth
            title: root.sectionTitle
            cells: root.cells
            moreAvailable: root.canSeeAll
            onMoreRequested: Browser.open(root.moreDestination)
            onCellActivated: index => Browser.activate(root.entries, index)
        }
    }

    Component {
        id: destinations

        Flow {
            width: root.usableWidth
            spacing: 10
            bottomPadding: 12

            Repeater {
                model: root.cells

                delegate: DestinationCard {
                    id: destination

                    required property var modelData
                    required property int index

                    width: Math.max(220, (root.usableWidth - 20) / 3)
                    title: destination.modelData.title
                    icon: destination.modelData.glyph.length > 0 ? destination.modelData.glyph : "explore"
                    onClicked: Browser.activate(root.entries, destination.index)
                }
            }
        }
    }

    Component {
        id: chips

        Flow {
            width: root.usableWidth
            spacing: 6
            bottomPadding: 12

            Repeater {
                model: root.cells

                delegate: PillButton {
                    id: chip

                    required property var modelData
                    required property int index

                    text: chip.modelData.title
                    horizontalPadding: 16
                    onClicked: Browser.activate(root.entries, chip.index)
                }
            }
        }
    }

    Component {
        id: continuation

        Item {
            width: root.usableWidth
            implicitHeight: root.moreAvailable || root.busy ? 52 : 20

            BusySpinner {
                anchors.centerIn: parent
                visible: root.busy
            }

            ErrorState {
                anchors.centerIn: parent
                width: Math.min(560, parent.width)
                height: 52
                size: ErrorState.Inline
                shown: root.moreAvailable && !root.busy && !!root.page && root.page.error.length > 0
                busy: Connectivity.checking
                icon: !!root.page && root.page.unreachable ? "cloud_off" : "error"
                title: !!root.page && root.page.unreachable ? qsTr("You're offline")
                    : qsTr("Couldn't load more")
                onActionTriggered: Browser.retry()
            }
        }
    }

    Timer {
        interval: 80
        running: root.rowType === "continuation" && root.moreAvailable && !root.busy
            && root.nearViewport && root.visible && root.page && !root.page.loading
            && root.page.error.length === 0
        onTriggered: root.page.loadMore(root.sectionIndex)
    }
}
