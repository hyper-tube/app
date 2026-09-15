import QtQuick
import QtQuick.Controls
import HtMusic

Rectangle {
    id: root

    readonly property real contentWidth: Math.min(root.width, Theme.size.contentMaxWidth - Theme.size.bleed * 2)
    readonly property bool showShuffleRepeat: root.contentWidth >= 700
    readonly property bool showRating: root.contentWidth >= 900
    readonly property bool wideVolume: root.contentWidth >= 1010
    readonly property bool showTime: root.contentWidth >= 1150
    readonly property bool loaded: PlaybackController.track.valid
    readonly property bool episode: PlaybackController.episode
    readonly property real musicPresence: Math.max(0, Math.min(1, 1 - root.podcastReveal))
    readonly property real podcastPresence: Math.max(0, Math.min(1, root.podcastReveal))

    property real podcastReveal: root.episode ? 1 : 0

    signal expandRequested

    function openTrackMenu(x) {
        trackMenu.show({ "entry": PlaybackController.currentEntry(), "source": root, "x": x, "y": 0,
                         "selectable": false, "above": true });
    }

    implicitHeight: Theme.size.playerBar
    radius: Theme.rounding.normal
    color: Theme.colLayer2

    Behavior on podcastReveal {
        NumberAnimation {
            duration: Theme.duration.spatial
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.emphasized
        }
    }

    TrackSwitch {
        id: switcher

        vertical: true
        travel: 14
        dip: 0.16
    }

    SeekBar {
        id: seek

        anchors.top: parent.top
        anchors.topMargin: 6
        anchors.horizontalCenter: parent.horizontalCenter
        width: root.contentWidth - 36
        trackHeight: 4
        activeTrackHeight: 8
        position: PlaybackController.position
        duration: PlaybackController.duration
        live: PlaybackController.live
        enabled: root.loaded
        opacity: root.loaded ? 1 : 0.35
        onSeeked: milliseconds => PlaybackController.seek(milliseconds)
    }

    Item {
        id: body

        anchors.top: seek.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        width: root.contentWidth - 28

        Row {
            id: transport

            anchors.verticalCenter: parent.verticalCenter
            x: Math.max(0, Math.min((body.width - width) / 2, trailing.x - width - 12))
            spacing: 0

            TapHandler {
                acceptedButtons: Qt.RightButton
                gesturePolicy: TapHandler.ReleaseWithinBounds
            }

            Item {
                anchors.verticalCenter: parent.verticalCenter
                width: root.showShuffleRepeat ? 44 * root.musicPresence : 0
                height: 40
                visible: width > 0.5

                IconButton {
                    anchors.verticalCenter: parent.verticalCenter
                    icon: "shuffle"
                    toggled: PlaybackController.shuffle
                    opacity: root.musicPresence
                    scale: 0.5 + 0.5 * root.musicPresence
                    interactive: !root.episode
                    Accessible.name: qsTr("Shuffle")
                    onClicked: PlaybackController.shuffle = !PlaybackController.shuffle
                }
            }

            SkipButton {
                anchors.verticalCenter: parent.verticalCenter
                morph: root.podcastReveal
                Accessible.name: root.episode ? qsTr("Back 10 seconds") : qsTr("Previous track")
                interactive: root.episode ? root.loaded : PlaybackController.canGoPrevious
                opacity: interactive ? 1 : 0.35
                onClicked: {
                    if (root.episode)
                        PlaybackController.skip(-10000);
                    else
                        PlaybackController.previous();
                }
            }

            Item {
                width: 4
                height: 1
            }

            PlayPauseButton {
                anchors.verticalCenter: parent.verticalCenter
                playing: PlaybackController.playing
                iconSize: 28
                diameter: 50
                interactive: root.loaded
                opacity: interactive ? 1 : 0.35
                onClicked: PlaybackController.toggle()
            }

            Item {
                width: 4
                height: 1
            }

            SkipButton {
                anchors.verticalCenter: parent.verticalCenter
                forward: true
                morph: root.podcastReveal
                Accessible.name: root.episode ? qsTr("Forward 30 seconds") : qsTr("Next track")
                interactive: root.episode ? root.loaded : PlaybackController.canGoNext
                opacity: interactive ? 1 : 0.35
                onClicked: {
                    if (root.episode)
                        PlaybackController.skip(30000);
                    else
                        PlaybackController.next();
                }
            }

            Item {
                anchors.verticalCenter: parent.verticalCenter
                width: root.showShuffleRepeat ? 44 * root.musicPresence : 0
                height: 40
                visible: width > 0.5

                IconButton {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    icon: PlaybackController.repeat === 2 ? "repeat_one" : "repeat"
                    toggled: PlaybackController.repeat > 0
                    opacity: root.musicPresence
                    scale: 0.5 + 0.5 * root.musicPresence
                    interactive: !root.episode
                    Accessible.name: qsTr("Repeat")
                    onClicked: PlaybackController.cycleRepeat()
                }
            }
        }

        Row {
            id: trailing

            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            spacing: 6

            TapHandler {
                acceptedButtons: Qt.RightButton
                gesturePolicy: TapHandler.ReleaseWithinBounds
            }

            StyledText {
                anchors.verticalCenter: parent.verticalCenter
                visible: root.showTime && root.loaded && !PlaybackController.live
                text: PlaybackController.formatTime(PlaybackController.position)
                    + " / " + PlaybackController.formatTime(PlaybackController.duration)
                font.pixelSize: Theme.font.smaller
                color: Theme.colInactive
            }

            LiveBadge {
                anchors.verticalCenter: parent.verticalCenter
                width: implicitWidth
                height: implicitHeight
                visible: PlaybackController.live
                filled: false
                pixelSize: Theme.font.smaller
                pulsing: PlaybackController.playing
            }

            LikeButton {
                anchors.verticalCenter: parent.verticalCenter
                visible: root.showRating && Account.signedIn && PlaybackController.track.valid
                videoId: PlaybackController.track.videoId
                liked: PlaybackController.track.liked
                disliked: PlaybackController.track.disliked
            }

            LikeButton {
                anchors.verticalCenter: parent.verticalCenter
                visible: root.showRating && Account.signedIn && PlaybackController.track.valid
                down: true
                videoId: PlaybackController.track.videoId
                liked: PlaybackController.track.liked
                disliked: PlaybackController.track.disliked
            }

            Item {
                anchors.verticalCenter: parent.verticalCenter
                width: tune.width * root.musicPresence + (speed.width + 4) * root.podcastPresence
                height: 40
                clip: true

                IconButton {
                    id: tune

                    anchors.verticalCenter: parent.verticalCenter
                    icon: "graphic_eq"
                    toggled: PlaybackSettings.equalizerEnabled
                        || PlaybackSettings.transitionMode !== PlaybackSettings.TransitionsOff
                    opacity: root.musicPresence
                    scale: 0.5 + 0.5 * root.musicPresence
                    interactive: !root.episode
                    Accessible.name: qsTr("Sound")
                    onClicked: tuning.toggle()
                }

                SpeedButton {
                    id: speed

                    x: 2
                    anchors.verticalCenter: parent.verticalCenter
                    opacity: root.podcastPresence
                    scale: 0.6 + 0.4 * root.podcastPresence
                    interactive: root.episode
                }
            }

            VolumeControl {
                anchors.verticalCenter: parent.verticalCenter
                compact: !root.wideVolume
            }

            IconButton {
                anchors.verticalCenter: parent.verticalCenter
                icon: "expand_less"
                Accessible.name: qsTr("Expand player")
                onClicked: root.expandRequested()
            }
        }

        Row {
            id: identity

            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: Math.max(0, Math.min(300, transport.x - 20))
            height: body.height
            visible: width >= 64
            spacing: 12

            Item {
                id: cover

                anchors.verticalCenter: parent.verticalCenter
                width: 52
                height: 52
                opacity: switcher.fade
                scale: switcher.depth

                Artwork {
                    anchors.fill: parent
                    artId: switcher.track.artId
                    rounding: Theme.rounding.verysmall
                }
            }

            Item {
                anchors.verticalCenter: parent.verticalCenter
                width: Math.max(0, parent.width - cover.width - parent.spacing)
                height: parent.height
                visible: width > 8
                clip: true

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width
                    spacing: 2
                    opacity: switcher.fade
                    transform: Translate { y: switcher.offsetY }

                    Marquee {
                        width: parent.width
                        text: switcher.track.valid ? switcher.track.title : qsTr("Nothing playing")
                        title: switcher.track.valid
                        color: switcher.track.valid ? Theme.colOnSurface : Theme.colInactive
                        pixelSize: Theme.font.small
                        active: PlaybackController.playing
                    }

                    StyledText {
                        width: parent.width
                        visible: switcher.track.valid
                        text: switcher.track.artist
                        font.pixelSize: Theme.font.smaller
                        color: Theme.colInactive
                        elide: Text.ElideRight
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

    TuningPopup {
        id: tuning

        parent: tune
        x: tune.width / 2 - width + 40
        y: -height - 10
        anchorX: (tuning.width - 40) / tuning.width
        anchorY: 1
    }

    TapHandler {
        enabled: root.loaded
        acceptedButtons: Qt.RightButton
        gesturePolicy: TapHandler.ReleaseWithinBounds
        onTapped: eventPoint => root.openTrackMenu(eventPoint.position.x)
    }
}
