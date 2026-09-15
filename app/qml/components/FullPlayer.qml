import QtQuick
import QtQuick.Controls
import HtMusic

Rectangle {
    id: root

    property bool open: false
    property int pane: 1
    property real podcastReveal: root.episode ? 1 : 0

    readonly property alias backgroundItem: ambience
    readonly property bool episode: PlaybackController.episode
    readonly property real musicPresence: Math.max(0, Math.min(1, 1 - root.podcastReveal))
    readonly property real podcastPresence: Math.max(0, Math.min(1, root.podcastReveal))
    readonly property bool lyricsShown: !root.episode && root.pane === 0
    readonly property bool queueShown: root.episode ? root.pane === 0 : root.pane === 1
    readonly property bool detailsShown: root.episode && root.pane === 1

    signal collapseRequested

    y: root.height
    visible: y < height
    radius: Theme.rounding.large
    color: Theme.colLayer0

    Behavior on podcastReveal {
        NumberAnimation {
            duration: Theme.duration.spatial
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.emphasized
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.AllButtons
        onWheel: event => event.accepted = true
    }

    TrackSwitch {
        id: switcher

        travel: 64
        dip: 0.08
    }

    states: State {
        name: "open"
        when: root.open

        PropertyChanges {
            target: root
            y: 0
        }
    }

    transitions: [
        Transition {
            to: "open"
            NumberAnimation {
                property: "y"
                duration: Theme.duration.sheetEnter
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.emphasized
            }
        },
        Transition {
            from: "open"
            NumberAnimation {
                property: "y"
                duration: Theme.duration.sheetExit
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.emphasized
            }
        }
    ]

    Item {
        id: ambience

        anchors.fill: parent
        layer.enabled: true
        layer.effect: RoundedMask {
            rounding: root.radius
        }

        AmbientArt {
            anchors.fill: parent
            artId: switcher.track.artId
            opacity: 0.65
        }

        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0; color: ColorUtils.withAlpha(Theme.colLayer0, 0.42) }
                GradientStop { position: 1.0; color: ColorUtils.withAlpha(Theme.colLayer0, 0.88) }
            }
        }
    }

    Item {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: 110
        anchors.bottomMargin: 40
        width: Math.min(parent.width, Theme.size.contentMaxWidth) - 80

        Column {
            id: stage

            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: Math.min(400, parent.width * 0.4)
            spacing: 16

            Column {
                id: face

                width: parent.width
                spacing: 16
                opacity: switcher.fade
                scale: switcher.depth
                transform: Translate { x: switcher.offsetX }

                VideoStage {
                    anchors.horizontalCenter: parent.horizontalCenter
                    artId: switcher.track.artId
                    video: switcher.track.video
                    live: switcher.track.live
                    reach: Math.max(100, stage.parent.height - 310)
                    span: stage.width
                }

                Column {
                    width: parent.width
                    spacing: 4

                    Marquee {
                        width: parent.width
                        text: switcher.track.title
                        title: true
                        pixelSize: Theme.font.display
                        active: PlaybackController.playing
                    }

                    StyledText {
                        width: parent.width
                        text: switcher.track.artist
                            + (switcher.track.album ? "  -  " + switcher.track.album : "")
                        font.pixelSize: Theme.font.large
                        color: Theme.colOnSurfaceVariant
                        elide: Text.ElideRight
                    }
                }
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: PlaybackController.track.valid
                spacing: 12

                LikeButton {
                    visible: Account.signedIn
                    videoId: PlaybackController.track.videoId
                    liked: PlaybackController.track.liked
                    disliked: PlaybackController.track.disliked
                }

                LikeButton {
                    visible: Account.signedIn
                    down: true
                    videoId: PlaybackController.track.videoId
                    liked: PlaybackController.track.liked
                    disliked: PlaybackController.track.disliked
                }

                IconButton {
                    readonly property int downloadState: {
                        Downloads.ids;
                        return Downloads.stateOf(PlaybackController.track.videoId);
                    }

                    visible: !PlaybackController.live
                    icon: downloadState === Downloads.Ready ? "download_done" : "download"
                    toggled: downloadState === Downloads.Ready
                    Accessible.name: downloadState === Downloads.Ready ? qsTr("Remove download")
                        : qsTr("Download track")
                    interactive: PlaybackController.track.valid
                        && downloadState !== Downloads.Waiting && downloadState !== Downloads.Fetching
                    onClicked: {
                        if (downloadState === Downloads.Ready)
                            Downloads.discard([PlaybackController.track]);
                        else
                            Downloads.keep([PlaybackController.track]);
                    }
                }

                IconButton {
                    id: trackActions

                    icon: "more_vert"
                    toggled: trackMenu.opened
                    Accessible.name: qsTr("More actions")
                    onClicked: trackMenu.show({ "entry": PlaybackController.currentEntry(),
                                                "source": trackActions, "x": trackActions.width / 2,
                                                "y": trackActions.height, "selectable": false })
                }
            }

            Column {
                width: parent.width
                spacing: 2

                SeekBar {
                    width: parent.width
                    position: PlaybackController.position
                    duration: PlaybackController.duration
                    live: PlaybackController.live
                    onSeeked: milliseconds => PlaybackController.seek(milliseconds)
                }

                Item {
                    width: parent.width
                    height: 18

                    StyledText {
                        anchors.left: parent.left
                        visible: !PlaybackController.live
                        text: PlaybackController.formatTime(PlaybackController.position)
                        font.pixelSize: Theme.font.smaller
                        color: Theme.colInactive
                    }

                    StyledText {
                        anchors.right: parent.right
                        visible: !PlaybackController.live
                        text: PlaybackController.formatTime(PlaybackController.duration)
                        font.pixelSize: Theme.font.smaller
                        color: Theme.colInactive
                    }

                    StyledText {
                        anchors.left: parent.left
                        visible: PlaybackController.live
                        text: qsTr("Live station")
                        title: true
                        font.pixelSize: Theme.font.smaller
                        color: Theme.colError
                    }
                }
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 8

                Item {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 40 * root.musicPresence + fullSpeed.width * root.podcastPresence
                    height: 40

                    IconButton {
                        anchors.centerIn: parent
                        icon: "shuffle"
                        opacity: root.musicPresence
                        scale: 0.5 + 0.5 * root.musicPresence
                        interactive: !root.episode
                        Accessible.name: PlaybackController.shuffle ? qsTr("Turn shuffle off")
                            : qsTr("Turn shuffle on")
                        toggled: PlaybackController.shuffle
                        onClicked: PlaybackController.shuffle = !PlaybackController.shuffle
                    }

                    SpeedButton {
                        id: fullSpeed

                        anchors.centerIn: parent
                        glass: true
                        opacity: root.podcastPresence
                        scale: 0.6 + 0.4 * root.podcastPresence
                        interactive: root.episode
                    }
                }

                SkipButton {
                    anchors.verticalCenter: parent.verticalCenter
                    morph: root.podcastReveal
                    iconSize: 30
                    diameter: 48
                    Accessible.name: root.episode ? qsTr("Back 10 seconds") : qsTr("Previous track")
                    onClicked: {
                        if (root.episode)
                            PlaybackController.skip(-10000);
                        else
                            PlaybackController.previous();
                    }
                }

                PlayPauseButton {
                    anchors.verticalCenter: parent.verticalCenter
                    playing: PlaybackController.playing
                    iconSize: 34
                    diameter: 66
                    onClicked: PlaybackController.toggle()
                }

                SkipButton {
                    anchors.verticalCenter: parent.verticalCenter
                    forward: true
                    morph: root.podcastReveal
                    iconSize: 30
                    diameter: 48
                    Accessible.name: root.episode ? qsTr("Forward 30 seconds") : qsTr("Next track")
                    onClicked: {
                        if (root.episode)
                            PlaybackController.skip(30000);
                        else
                            PlaybackController.next();
                    }
                }

                Item {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 40 * root.musicPresence + fullSpeed.width * root.podcastPresence
                    height: 40

                    IconButton {
                        anchors.centerIn: parent
                        icon: PlaybackController.repeat === 2 ? "repeat_one" : "repeat"
                        opacity: root.musicPresence
                        scale: 0.5 + 0.5 * root.musicPresence
                        interactive: !root.episode
                        Accessible.name: PlaybackController.repeat === 2 ? qsTr("Repeat one track")
                            : PlaybackController.repeat === 1 ? qsTr("Repeat all tracks")
                            : qsTr("Repeat off")
                        toggled: PlaybackController.repeat > 0
                        onClicked: PlaybackController.cycleRepeat()
                    }

                    IconButton {
                        anchors.centerIn: parent
                        icon: "skip_next"
                        iconFill: 1
                        opacity: root.podcastPresence
                        scale: 0.5 + 0.5 * root.podcastPresence
                        interactive: root.episode && PlaybackController.canGoNext
                        Accessible.name: qsTr("Next episode")
                        onClicked: PlaybackController.next()
                    }
                }
            }
        }

        Item {
            anchors.left: stage.right
            anchors.leftMargin: 44 + Math.min(68, Math.max(0, (parent.width - 1100) * 0.2))
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.topMargin: 0

            GlassSurface {
                anchors.fill: tabs
                sourceItem: ambience
            }

            SegmentedTabs {
                id: tabs

                anchors.horizontalCenter: parent.horizontalCenter
                width: Math.min(260, parent.width)
                labels: root.episode ? [qsTr("Up next"), qsTr("Details")] : [qsTr("Lyrics"), qsTr("Up next")]
                glass: true
                current: root.pane
                onCurrentChanged: root.pane = current
            }

            Item {
                anchors.top: tabs.bottom
                anchors.topMargin: 18
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom

                LyricsPane {
                    anchors.fill: parent
                    enabled: root.lyricsShown
                    opacity: root.lyricsShown ? 1 : 0
                    visible: opacity > 0

                    Behavior on opacity {
                        NumberAnimation {
                            duration: Theme.duration.fast
                            easing.type: Easing.BezierSpline
                            easing.bezierCurve: Theme.curve.expressiveEffects
                        }
                    }
                }

                QueuePane {
                    anchors.fill: parent
                    enabled: root.queueShown
                    onSourceRequested: source => {
                        root.collapseRequested();
                        Browser.open(source);
                    }
                    opacity: root.queueShown ? 1 : 0
                    visible: opacity > 0

                    Behavior on opacity {
                        NumberAnimation {
                            duration: Theme.duration.fast
                            easing.type: Easing.BezierSpline
                            easing.bezierCurve: Theme.curve.expressiveEffects
                        }
                    }
                }

                EpisodeDetailsPane {
                    anchors.fill: parent
                    enabled: root.detailsShown
                    opacity: root.detailsShown ? 1 : 0
                    visible: opacity > 0
                    onPodcastRequested: browseId => {
                        root.collapseRequested();
                        Browser.openPage(browseId, "podcast");
                    }

                    Behavior on opacity {
                        NumberAnimation {
                            duration: Theme.duration.fast
                            easing.type: Easing.BezierSpline
                            easing.bezierCurve: Theme.curve.expressiveEffects
                        }
                    }
                }
            }
        }
    }

    ItemMenu {
        id: trackMenu

        parent: Overlay.overlay
        builder: entry => Actions.buildPlayer(entry)
    }

    IconButton {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 18
        anchors.topMargin: 76
        colBackground: ColorUtils.withAlpha(Theme.colLayer3, 0.65)
        icon: "expand_more"
        Accessible.name: qsTr("Collapse player")
        iconSize: 26
        onClicked: root.collapseRequested()
    }

    Component.onCompleted: root.pane = root.episode ? 0 : 1
    onEpisodeChanged: root.pane = root.episode ? 0 : 1
}
