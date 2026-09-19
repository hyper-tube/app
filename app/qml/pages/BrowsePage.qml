import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import HtMusic

Item {
    id: root

    property BrowseModel page: Browser.page
    property real contentReveal: 1
    property bool contentReady: true
    property bool positioned: false
    property real pinnedY: 0
    property real loadingReveal: 1
    property bool selectMode: false
    property var selection: ({})
    property int selectionCount: 0
    property ItemModel selectionModel: null
    property var menuRequest: null

    readonly property real pageWidth: Math.min(root.width, Theme.size.contentMaxWidth)
    readonly property real pageInset: (root.width - root.pageWidth) / 2
    readonly property real contentWidth: Math.min(list.width, Theme.size.contentMaxWidth)
    readonly property real cellSize: Math.min(200,
        (root.contentWidth - Theme.size.bleed * 2) / contentModel.columns - 24)
    readonly property real podcastColumn: Math.max(280, Math.min(380, root.pageWidth * 0.3))
    readonly property bool podcastSplit: root.podcastPage && root.pageWidth >= 1000
    readonly property bool initialLoading: !!root.page && root.page.refreshing
    readonly property bool modalActive: newPlaylist.modalVisible || deletePlaylist.modalVisible
        || editor.open || PlaylistTargets.open
    readonly property bool libraryLocked: root.libraryPage && !Account.signedIn
    readonly property bool offlineSearch: !!page && page.local && page.kind === "search"
    readonly property bool downloadPage: !!page && page.kind === "downloads"
    readonly property bool offline: !!page && page.unreachable
    readonly property bool playlistPage: !!page && (page.kind === "playlist" || page.kind === "podcast")
    readonly property bool podcastPage: !!page && page.kind === "podcast"
    readonly property bool libraryPage: !!page && page.kind === "library"
    readonly property bool historyPage: !!page && page.kind === "history"
    readonly property bool filteredLibrary: root.libraryPage && Browser.libraryTab !== 0
        && root.page.filterable
    readonly property bool filtered: !!root.page && root.page.filter.length > 0

    Column {
        id: libraryHeader

        anchors.horizontalCenter: parent.horizontalCenter
        width: root.contentWidth - Theme.size.bleed * 2
        enabled: !root.modalActive
        visible: root.libraryPage
        spacing: 18

        Item {
            width: parent.width
            height: Math.max(libraryTitle.implicitHeight, librarySort.implicitHeight)

            SectionHeader {
                id: libraryTitle

                anchors.left: parent.left
                anchors.right: librarySort.left
                anchors.rightMargin: 16
                anchors.verticalCenter: parent.verticalCenter
                title: qsTr("Library")
            }

            BrowseFilter {
                id: librarySort

                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                visible: Account.signedIn && !!root.page && root.page.sortable
                page: root.page
            }
        }

        Item {
            width: parent.width
            height: Math.max(tabFlow.implicitHeight, actions.height)

            Flow {
                id: tabFlow

                anchors.left: parent.left
                anchors.right: actions.left
                anchors.rightMargin: actions.width > 0 ? 20 : 0
                spacing: 6

                Repeater {
                    model: Browser.libraryTabs

                    delegate: PillButton {
                        required property var modelData
                        required property int index

                        text: modelData.title
                        horizontalPadding: 14
                        toggled: Browser.libraryTab === index
                        onClicked: Browser.showLibraryTab(index)
                    }
                }
            }

            Row {
                id: actions

                anchors.right: parent.right
                spacing: 0

                RevealingSelect {
                    anchors.verticalCenter: parent.verticalCenter
                    width: implicitWidth
                    gap: 8
                    shown: root.filteredLibrary
                    choices: root.filteredLibrary ? root.page.chips : []
                    current: root.filteredLibrary ? root.page.filterIndex : -1
                    interactive: root.filteredLibrary && !root.page.loading
                    onPicked: index => Browser.chooseChip(index)
                }

                RevealingButton {
                    shown: Account.signedIn && Browser.uploadsTab
                    text: qsTr("Upload")
                    icon: "upload"
                    iconSize: 20
                    leadingPadding: 12
                    onClicked: chooser.open()
                }

                RevealingButton {
                    shown: Account.signedIn && Browser.playlistsTab
                    text: qsTr("New playlist")
                    icon: "add"
                    iconSize: 20
                    leadingPadding: 12
                    onClicked: newPlaylist.open = true
                }
            }
        }
    }

    FileDialog {
        id: chooser

        title: qsTr("Upload music")
        fileMode: FileDialog.OpenFiles
        nameFilters: [qsTr("Audio files (%1)").arg(Uploads.extensions.join(" "))]
        onAccepted: Uploads.add(selectedFiles)
    }

    BrowseContentModel {
        id: contentModel

        page: root.page
        columns: Math.max(1, Math.floor((root.contentWidth - Theme.size.bleed * 2) / 188))
    }

    Flickable {
        id: podcastColumn

        x: root.pageInset + Theme.size.bleed
        width: root.podcastColumn
        height: parent.height
        visible: root.podcastSplit
        enabled: !root.modalActive
        contentWidth: width
        contentHeight: podcastHeader.implicitHeight + 24
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        interactive: contentHeight > height
        opacity: root.contentReveal
        transform: Translate { y: 12 * (1 - root.contentReveal) }

        PodcastHeader {
            id: podcastHeader

            width: podcastColumn.width
            page: root.podcastSplit ? root.page : null
            onMenuRequested: request => itemMenu.show(request)
        }
    }

    ListView {
        id: list

        anchors.top: root.libraryPage ? libraryHeader.bottom : parent.top
        anchors.topMargin: root.libraryPage ? 24 : 0
        anchors.left: root.podcastSplit ? podcastColumn.right : parent.left
        anchors.leftMargin: root.podcastSplit ? 28 - Theme.size.bleed : 0
        anchors.right: parent.right
        anchors.rightMargin: root.podcastSplit ? root.pageInset : 0
        anchors.bottom: parent.bottom
        clip: true
        spacing: 2
        enabled: !root.modalActive
        visible: !root.libraryLocked
        opacity: root.contentReveal
        transform: Translate { y: 12 * (1 - root.contentReveal) }
        boundsBehavior: Flickable.StopAtBounds
        acceptedButtons: Qt.NoButton
        reuseItems: false
        cacheBuffer: 240
        model: root.contentReady ? contentModel : null

        ScrollBar.vertical: PageScrollBar {}

        remove: Transition {
            enabled: root.playlistPage && !root.initialLoading
            NumberAnimation {
                property: "removal"
                to: 1
                duration: Theme.duration.exit
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.emphasizedAccel
            }
        }

        removeDisplaced: Transition {
            enabled: root.playlistPage && !root.initialLoading
            SequentialAnimation {
                PauseAnimation { duration: Theme.duration.exit }
                NumberAnimation {
                    property: "y"
                    duration: Theme.duration.spatial
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Theme.curve.expressiveDefaultSpatial
                }
            }
        }

        move: Transition {
            enabled: !playlistDrag.suppressMoves
            NumberAnimation {
                property: "y"
                duration: Theme.duration.spatial
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveDefaultSpatial
            }
        }

        moveDisplaced: Transition {
            enabled: !playlistDrag.suppressMoves
            NumberAnimation {
                property: "y"
                duration: Theme.duration.spatial
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveDefaultSpatial
            }
        }

        header: Item {
            width: list.width
            height: root.libraryPage ? 0 : pageHeader.implicitHeight
            visible: !root.libraryPage
            onHeightChanged: root.keepTop()

            BrowseHeader {
                id: pageHeader

                x: (list.width - root.contentWidth) / 2 + Theme.size.bleed
                width: root.contentWidth - Theme.size.bleed * 2
                page: root.page
                podcastInline: !root.podcastSplit
                onMenuRequested: request => itemMenu.show(request)
                onFilterDismissed: root.forceActiveFocus()
            }
        }

        delegate: BrowseRow {
            enabled: !root.initialLoading && removal === 0
            width: list.width
            contentWidth: root.contentWidth
            reveal: Math.max(0, Math.min(1, (root.loadingReveal
                - Math.min(0.25, Math.max(0, y - list.contentY) / Math.max(1, list.height) * 0.25)) / 0.75))
            page: root.page
            cellSize: root.cellSize
            viewport: list
            reorder: playlistDrag
            selectMode: root.selectMode
            selection: root.selection
            onMenuRequested: request => {
                root.menuRequest = request;
                root.selectionModel = request.model;
                itemMenu.show(request);
            }
            onSelectionToggled: (model, index) => root.toggleSelection(model, index)
        }

        footer: Item {
            width: list.width
            height: footerContent.implicitHeight

            Column {
                id: footerContent

                readonly property real freeHeight: list.height
                    - (list.headerItem ? list.headerItem.height : 0)
                    - footerContent.spacing - trailing.height

                x: (list.width - root.contentWidth) / 2 + Theme.size.bleed
                width: root.contentWidth - Theme.size.bleed * 2
                spacing: 16
                opacity: root.loadingReveal

                BusySpinner {
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: root.page && root.page.loading && root.page.count > 0
                }

                ErrorState {
                    readonly property bool blank: !!root.page && root.page.count === 0
                    readonly property bool keepable: root.offline && Downloads.ids.length > 0

                    width: parent.width
                    height: blank ? Math.max(320, footerContent.freeHeight) : implicitHeight
                    size: blank ? ErrorState.Page : ErrorState.Inline
                    shown: !!root.page && root.page.error.length > 0 && !root.page.loading
                    busy: Connectivity.checking
                    icon: root.offline ? "cloud_off" : "error"
                    title: root.offline ? qsTr("You're offline")
                        : blank ? qsTr("Something went wrong") : qsTr("Couldn't load more")
                    caption: !blank ? ""
                        : keepable ? qsTr("Connect to the internet to load this page. Your downloads are still here.")
                        : root.offline ? qsTr("Connect to the internet to load this page.")
                        : qsTr("This page didn't load. Try again in a moment.")
                    secondaryText: blank && keepable ? qsTr("Go to downloads") : ""
                    secondaryIcon: "download_for_offline"
                    onActionTriggered: Browser.retry()
                    onSecondaryTriggered: Browser.showDownloads()
                }

                EmptyState {
                    width: parent.width
                    height: Math.max(220, footerContent.freeHeight)
                    shown: !!root.page && !root.page.loading && root.page.error.length === 0
                        && !root.page.messaged
                        && ((root.playlistPage ? !root.page.playable : root.page.count === 0)
                            || (root.filtered && contentModel.matchCount === 0))
                    icon: root.filtered || root.offlineSearch ? "search_off" : root.downloadPage ? "download_for_offline" : root.historyPage ? "history" : root.libraryPage ? "library_music" : root.podcastPage ? "podcasts" : root.playlistPage ? "queue_music" : "search"
                    title: root.filtered ? qsTr("No matches")
                        : root.offlineSearch ? qsTr("No downloads match")
                        : root.downloadPage ? qsTr("No downloads yet")
                        : root.historyPage ? qsTr("No history yet")
                        : root.libraryPage ? qsTr("Nothing here yet")
                        : root.podcastPage ? qsTr("No episodes")
                        : root.playlistPage ? qsTr("This playlist is empty")
                        : qsTr("No results")
                    caption: root.filtered ? (root.downloadPage ? qsTr("Nothing in your downloads matches that")
                            : root.podcastPage ? qsTr("Nothing in this show matches that")
                            : qsTr("Nothing in this playlist matches that"))
                        : root.offlineSearch ? qsTr("Connect to the internet to search all of YouTube Music")
                        : root.downloadPage ? qsTr("Download a track or enable smart downloads in Settings")
                        : root.historyPage ? qsTr("Songs and episodes you play show up here")
                        : root.libraryPage ? qsTr("Save music on any device and it shows up here")
                        : root.podcastPage ? qsTr("Episodes of this show appear here")
                        : root.playlistPage ? qsTr("Add songs from a track's menu to start this playlist")
                        : qsTr("Nothing to show here yet")
                }

                Item {
                    id: trailing

                    width: parent.width
                    height: root.selectMode ? 96 : 20
                }
            }
        }

        onCountChanged: {
            if (!root.positioned && list.count > 0)
                root.showTop();
        }
        onContentYChanged: {
            if (root.positioned && Math.abs(list.contentY - list.originY) < 1)
                root.pinnedY = list.contentY;
            pagination.restart();
        }
        onContentHeightChanged: pagination.restart()
        onHeightChanged: pagination.restart()
    }

    PlaylistDrag {
        id: playlistDrag

        anchors.fill: list
        view: list
        page: root.page
        contentWidth: root.contentWidth
        visible: !root.modalActive && !root.libraryLocked
    }

    Item {
        anchors.fill: list
        clip: true
        visible: !root.libraryLocked && root.loadingReveal < 1
        opacity: root.contentReveal

        LoadingSkeleton {
            x: (list.width - root.contentWidth) / 2 + Theme.size.bleed
            y: list.headerItem ? list.headerItem.y + list.headerItem.height - list.contentY : 0
            width: root.contentWidth - Theme.size.bleed * 2
            shape: root.podcastPage ? "episodes" : !!root.page ? root.page.skeleton : "list"
            columns: contentModel.columns
            cellSize: root.cellSize
            departure: root.loadingReveal
        }
    }

    ScrollWheel {
        parent: list
        target: list
    }

    ItemMenu {
        id: itemMenu

        parent: root
    }

    SelectionBar {
        id: selectionBar

        enabled: !root.modalActive

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        active: root.selectMode
        count: root.selectionCount
        removable: !!root.page && root.page.owned
        onCancelRequested: root.endSelection()
        onSaveRequested: PlaylistTargets.show(root.selectedVideoIds())
        onRemoveRequested: root.removeSelected()
        onPlayNextRequested: {
            PlaybackController.playNext(root.selectedTracks());
            root.endSelection();
        }
        onQueueRequested: {
            PlaybackController.enqueue(root.selectedTracks());
            root.endSelection();
        }
        onDownloadRequested: {
            Downloads.keep(root.selectedTracks());
            root.endSelection();
        }
    }

    Timer {
        id: reattach

        interval: 0
        onTriggered: {
            root.contentReady = true;
            list.currentIndex = -1;
            root.showTop();
        }
    }

    Timer {
        id: pagination

        interval: 80
        onTriggered: {
            if (root.visible && root.page && root.page.hasMore && !root.page.loading
                && root.page.error.length === 0
                && list.contentY + list.height + 600 >= list.originY + list.contentHeight)
                root.page.loadMore();
        }
    }

    Connections {
        target: root.page

        function onReloadStarted() {
            root.endSelection();
            root.positioned = false;
            Qt.callLater(root.showTop);
        }

        function onStateChanged() {
            pagination.restart();
        }
    }

    Connections {
        target: Actions

        function onEditRequested(entry) {
            editor.load(entry);
        }

        function onDeleteRequested(entry) {
            root.confirmDelete(entry.title, entry.playlistTarget);
        }

        function onSelectRequested(entry) {
            root.beginSelection();
            if (root.menuRequest && root.menuRequest.index >= 0)
                root.toggleSelection(root.menuRequest.model, root.menuRequest.index);
        }
    }

    Connections {
        target: PlaylistTargets

        function onStateChanged() {
            if (!PlaylistTargets.open && root.selectMode)
                root.endSelection();
        }
    }

    Item {
        id: locked

        property real reveal: root.libraryLocked ? 1 : 0

        anchors.fill: list
        visible: locked.reveal > 0
        enabled: root.libraryLocked
        opacity: locked.reveal
        transform: Translate { y: 12 * (1 - locked.reveal) }

        Behavior on reveal {
            NumberAnimation {
                duration: Theme.duration.enter
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.emphasizedDecel
            }
        }

        Column {
            anchors.centerIn: parent
            spacing: 14

            EmptyState {
                width: 360
                shown: root.libraryLocked
                icon: "account_circle"
                title: qsTr("Your library")
                caption: Account.error.length > 0 ? Account.error
                    : Connectivity.online ? qsTr("Sign in to YouTube Music to see your saved music")
                    : qsTr("Connect to the internet to sign in. Your downloads are still here.")
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 8

                PillButton {
                    text: qsTr("Sign in")
                    icon: "login"
                    toggled: true
                    interactive: !Account.busy
                    onClicked: Account.requestSignIn()
                }

                PillButton {
                    visible: !Connectivity.online
                    text: qsTr("Go to downloads")
                    icon: "download_for_offline"
                    onClicked: Browser.showDownloads()
                }
            }
        }
    }

    TextPrompt {
        id: newPlaylist

        title: qsTr("New playlist")
        placeholder: qsTr("Playlist name")
        onAccepted: value => {
            LibraryActions.createPlaylist(value);
            open = false;
        }
        onDismissed: open = false
    }

    TextPrompt {
        id: deletePlaylist

        property string targetTitle
        property string targetId

        title: qsTr("Delete playlist?")
        message: qsTr("\"%1\" will be permanently deleted.").arg(targetTitle)
        showInput: false
        confirm: qsTr("Delete")
        onAccepted: {
            LibraryActions.deletePlaylist(targetId);
            open = false;
        }
        onDismissed: open = false
    }

    PlaylistEditor {
        id: editor

        anchors.fill: parent
        onCloseRequested: open = false
    }

    PlaylistPicker {
        anchors.fill: parent
    }

    NumberAnimation {
        id: reveal

        target: root
        property: "contentReveal"
        from: 0
        to: 1
        duration: Theme.duration.fast
        easing.type: Easing.OutCubic
    }

    states: State {
        name: "loading"
        when: root.initialLoading

        PropertyChanges {
            target: root
            loadingReveal: 0
        }
    }

    transitions: Transition {
        from: "loading"

        NumberAnimation {
            target: root
            property: "loadingReveal"
            duration: Theme.duration.spatial
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.standardDecel
        }
    }

    onModalActiveChanged: {
        if (!modalActive)
            root.forceActiveFocus();
    }

    function confirmDelete(title, playlistId) {
        if (playlistId.length === 0)
            return;
        deletePlaylist.targetTitle = title;
        deletePlaylist.targetId = playlistId;
        deletePlaylist.open = true;
    }

    function beginSelection() {
        playlistDrag.cancel();
        root.selection = {};
        root.selectionCount = 0;
        root.selectMode = true;
    }

    function endSelection() {
        root.selectMode = false;
        root.selection = {};
        root.selectionCount = 0;
    }

    function toggleSelection(model, index) {
        root.selectionModel = model;
        const next = Object.assign({}, root.selection);
        if (next[index] === true)
            delete next[index];
        else
            next[index] = true;
        root.selection = next;
        root.selectionCount = Object.keys(next).length;
        if (root.selectionCount === 0)
            root.endSelection();
    }

    function selectedRows() {
        return Object.keys(root.selection).map(key => parseInt(key, 10)).sort((a, b) => a - b);
    }

    function selectedTracks() {
        return root.selectionModel ? root.selectionModel.tracksAt(root.selectedRows()) : [];
    }

    function selectedVideoIds() {
        return root.selectedTracks().map(track => track.videoId);
    }

    function removeSelected() {
        if (!root.selectionModel || !root.page)
            return;
        const videoIds = [];
        const setVideoIds = [];
        for (const row of root.selectedRows()) {
            const entry = root.selectionModel.get(row);
            if (entry.actions.setVideoId.length === 0)
                continue;
            videoIds.push(entry.track.videoId);
            setVideoIds.push(entry.actions.setVideoId);
        }
        LibraryActions.removeFromPlaylist(root.page.playlistTarget, videoIds, setVideoIds);
        root.endSelection();
    }

    function showTop() {
        list.forceLayout();
        list.positionViewAtBeginning();
        list.contentY = list.originY;
        root.positioned = list.count > 0;
        root.pinnedY = list.contentY;
    }

    function keepTop() {
        if (root.positioned && Math.abs(list.contentY - root.pinnedY) < 1)
            root.showTop();
    }

    onPageChanged: {
        root.contentReady = false;
        root.positioned = false;
        root.endSelection();
        reattach.restart();
        reveal.restart();
        pagination.restart();
    }
}
