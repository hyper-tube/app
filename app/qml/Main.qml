import QtQuick
import QtQuick.Controls
import HtMusic

Window {
    id: window

    readonly property bool sheetVisible: settings.visible || login.visible || plugins.visible
        || release.visible
    readonly property int chromeZ: Overlay.overlay ? Overlay.overlay.z + 1 : 1

    x: WindowState.initialX
    y: WindowState.initialY
    width: WindowState.initialWidth
    height: WindowState.initialHeight
    minimumWidth: 1040
    minimumHeight: 680 + titleBar.height
    flags: WindowChrome.windowFlags
    visible: true
    color: Theme.colLayer0
    title: AppInfo.name

    onClosing: close => {
        if (SystemSettings.background !== SystemSettings.Unavailable && SystemSettings.closeToTray) {
            close.accepted = false;
            window.hide();
        }
    }

    Connections {
        target: Browser

        function onPageChanged() {
            search.follow(Browser.searchQuery);
        }

        function onPlayRequested(tracks, index, source) {
            PlaybackController.playQueue(tracks, index, source);
        }

        function onResolutionFailed(message) {
            Toasts.show(message, true);
        }

        function onTracksResolved(tracks, intent) {
            switch (intent) {
            case "next":
                PlaybackController.playNext(tracks);
                Toasts.show(qsTr("Playing %n songs next", "", tracks.length));
                break;
            case "queue":
                PlaybackController.enqueue(tracks);
                Toasts.show(qsTr("Added %n songs to queue", "", tracks.length));
                break;
            case "download":
                Downloads.keep(tracks);
                break;
            case "save":
                PlaylistTargets.show(tracks.map(track => track.videoId));
                break;
            }
        }
    }

    Connections {
        target: Downloads

        function onFeedback(message) {
            Toasts.show(message);
        }
    }

    Connections {
        target: Uploads

        function onFeedback(message, error) {
            Toasts.show(message, error);
        }
    }

    WindowTitleBar {
        id: titleBar

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: implicitHeight
        visible: height > 0
        z: window.chromeZ
    }

    FocusScope {
        id: shell

        anchors.fill: parent
        anchors.topMargin: titleBar.height
        focus: true

        Keys.onPressed: event => {
            if (pages.modalActive) {
                event.accepted = true;
                return;
            }
            if (settings.open) {
                if (event.key === Qt.Key_Escape)
                    settings.open = false;
                event.accepted = true;
                return;
            }
            switch (event.key) {
            case Qt.Key_Space:
                PlaybackController.toggle();
                break;
            case Qt.Key_Right:
                if (PlaybackController.episode)
                    PlaybackController.skip(30000);
                else
                    PlaybackController.next();
                break;
            case Qt.Key_Left:
                if (PlaybackController.episode)
                    PlaybackController.skip(-10000);
                else
                    PlaybackController.previous();
                break;
            case Qt.Key_Escape:
                if (login.open)
                    login.open = false;
                else if (fullPlayer.open)
                    fullPlayer.open = false;
                else
                    Browser.back();
                break;
            case Qt.Key_Slash:
                search.focusInput();
                break;
            case Qt.Key_P:
                fullPlayer.open = !fullPlayer.open;
                break;
            case Qt.Key_M:
                rail.expanded = !rail.expanded;
                break;
            default:
                if (event.key >= Qt.Key_1 && event.key <= Qt.Key_4)
                    Browser.showSection(event.key - Qt.Key_1);
                return;
            }
            event.accepted = true;
        }

        NavRail {
            id: rail

            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: implicitWidth
            enabled: !pages.modalActive && !window.sheetVisible
            current: Browser.section
            onNavigateRequested: index => {
                fullPlayer.open = false;
                Browser.showSection(index);
            }
            onPluginsRequested: plugins.open = true
            onSettingsRequested: settings.open = true
        }

        Item {
            id: content

            anchors.left: rail.right
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.leftMargin: Theme.size.gutter - Theme.size.bleed
            anchors.rightMargin: Theme.size.gutter - Theme.size.bleed
            anchors.bottomMargin: 16
            clip: true

            Item {
                id: topBar

                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: 16
                anchors.leftMargin: Theme.size.bleed
                anchors.rightMargin: Theme.size.bleed
                height: 46
                z: 2
                enabled: !pages.modalActive && !window.sheetVisible

                SearchBar {
                    id: search

                    anchors.left: reload.right
                    anchors.leftMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.min(540, parent.width - back.width - reload.width
                        - connection.width - notifications.width - account.width - 72)
                    placeholder: Connectivity.online ? qsTr("Search songs, albums, artists")
                        : qsTr("Search your downloads")
                    suggestionsEnabled: true
                    glass: fullPlayer.visible
                    onEditingFinished: shell.forceActiveFocus()
                    onAccepted: query => {
                        fullPlayer.open = false;
                        Browser.search(query);
                        shell.forceActiveFocus();
                    }
                }

                IconButton {
                    id: back

                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    icon: "arrow_back"
                    Accessible.name: qsTr("Back")
                    interactive: Browser.canGoBack
                    opacity: Browser.canGoBack ? 1 : 0.3
                    onClicked: {
                        fullPlayer.open = false;
                        Browser.back();
                    }
                }

                IconButton {
                    id: reload

                    anchors.left: back.right
                    anchors.leftMargin: 4
                    anchors.verticalCenter: parent.verticalCenter
                    icon: "refresh"
                    Accessible.name: qsTr("Reload page")
                    interactive: !!Browser.page && !Browser.page.loading
                    opacity: interactive ? 1 : 0.3
                    onClicked: Browser.reloadPage()
                }

                ConnectionChip {
                    id: connection

                    anchors.right: notifications.left
                    anchors.rightMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    width: implicitWidth
                    height: implicitHeight
                    z: 1
                    glass: fullPlayer.visible
                }

                ControlCenterButton {
                    id: notifications

                    anchors.right: account.left
                    anchors.rightMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    z: 1
                }

                AccountButton {
                    id: account

                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    z: 1
                    glass: fullPlayer.visible
                    onHistoryRequested: {
                        fullPlayer.open = false;
                        Browser.showHistory();
                    }
                }

            }

            PageView {
                id: pages

                anchors.top: topBar.bottom
                anchors.topMargin: 24
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: playerBar.top
                anchors.bottomMargin: 16
                enabled: !fullPlayer.visible && !window.sheetVisible
            }

            PlayerBar {
                id: playerBar

                anchors.horizontalCenter: parent.horizontalCenter
                width: Math.min(parent.width, Theme.size.contentMaxWidth) - Theme.size.bleed * 2
                anchors.bottom: parent.bottom
                enabled: !fullPlayer.visible && !pages.modalActive && !window.sheetVisible
                onExpandRequested: fullPlayer.open = true
            }

            FullPlayer {
                id: fullPlayer

                anchors.left: parent.left
                anchors.right: parent.right
                height: parent.height
                onCollapseRequested: open = false
            }
        }
    }

    ToastHost {
        anchors.right: parent.right
        anchors.rightMargin: Theme.size.gutter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 120
        width: Math.min(440, parent.width - 48)
        z: 20
    }

    Connections {
        target: PlaybackController

        function onErrorChanged() {
            Toasts.show(PlaybackController.error, true);
        }

        function onUnavailable(message) {
            Toasts.show(message, true);
        }

        function onRemoteQueueContinued() {
            Toasts.show(qsTr("Picked up where you left off on another device"));
        }
    }

    Connections {
        target: LibraryActions

        function onErrorChanged() {
            Toasts.show(LibraryActions.error, true);
        }

        function onFeedback(message) {
            Toasts.show(message);
        }
    }

    SettingsSheet {
        id: settings

        anchors.fill: parent
        anchors.topMargin: titleBar.height
        onCloseRequested: open = false
    }

    PluginsSheet {
        id: plugins

        anchors.fill: parent
        anchors.topMargin: titleBar.height
        onCloseRequested: open = false
    }

    ReleaseSheet {
        id: release

        anchors.fill: parent
        anchors.topMargin: titleBar.height
        onCloseRequested: open = false
    }

    LoginSheet {
        id: login

        anchors.fill: parent
        onCloseRequested: open = false
    }

    WindowResizeGrips {
        anchors.fill: parent
        z: window.chromeZ + 1
    }

    TrayMenu {}

    Connections {
        target: PluginRegistry

        function onPageRequested(id) {
            plugins.show(id);
        }
    }

    Connections {
        target: Updater

        function onDetailsRequested() {
            settings.open = false;
            release.open = true;
        }
    }

    Connections {
        target: Account

        function onErrorRaised() {
            Toasts.show(Account.error, true);
        }

        function onSignInRequested() {
            login.open = true;
        }

        function onSignInUnavailable() {
            Toasts.show(qsTr("Connect to the internet to sign in."), true);
        }
    }

}
