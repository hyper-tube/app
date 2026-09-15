import QtQuick
import HtMusic

RippleSurface {
    id: root

    required property var entry

    property bool active: false
    property bool downloaded: false
    property real presence: root.reachable ? 1 : 0.55
    property real startedReveal: root.started ? 1 : 0

    readonly property bool compact: root.width < 560
    readonly property bool reachable: Connectivity.online || root.downloaded
    readonly property bool playingNow: root.active && PlaybackController.playing
    readonly property bool cueVisible: root.hovered || root.active
    readonly property bool writable: Account.signedIn && Connectivity.online
    readonly property bool tracking: root.active && PlaybackController.duration > 0
        && PlaybackController.position > 0
    readonly property real progress: root.tracking
        ? PlaybackController.position / PlaybackController.duration
        : root.entry.progress / 100
    readonly property bool started: !root.entry.played && root.progress > 0.005 && root.progress < 0.995
    readonly property string remaining: root.tracking
        ? qsTr("%1 left").arg(PlaybackController.formatTime(PlaybackController.duration - PlaybackController.position))
        : root.entry.progressLabel

    signal playRequested
    signal menuRequested(real x, real y)

    implicitHeight: Math.max(cover.height, details.implicitHeight) + (root.compact ? 24 : 28)
    rounding: Theme.rounding.normal
    colState: Theme.colOnSurface

    Behavior on presence {
        NumberAnimation {
            duration: Theme.duration.fast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveEffects
        }
    }

    Behavior on startedReveal {
        NumberAnimation {
            duration: Theme.duration.spatialFast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveDefaultSpatial
        }
    }

    Item {
        id: cover

        x: root.compact ? 10 : 14
        y: root.compact ? 12 : 14
        width: root.compact ? 72 : 96
        height: width
        opacity: root.presence

        Artwork {
            anchors.fill: parent
            artId: root.entry.artId
            rounding: Theme.rounding.small
        }

        Rectangle {
            anchors.fill: parent
            radius: Theme.rounding.small
            color: ColorUtils.withAlpha(Theme.colLayer1, 0.62)
            opacity: root.cueVisible ? 1 : 0

            Behavior on opacity {
                NumberAnimation {
                    duration: Theme.duration.fast
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Theme.curve.expressiveEffects
                }
            }

            Sym {
                anchors.centerIn: parent
                opacity: root.playingNow ? 0 : 1
                scale: root.hovered ? 1 : 0.8
                text: "play_arrow"
                iconSize: root.compact ? 28 : 34
                fill: 1
                color: Theme.colOnSurface

                Behavior on scale {
                    NumberAnimation {
                        duration: Theme.duration.spatialFast
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: Theme.curve.expressiveFastSpatial
                    }
                }
            }

            PlayingIndicator {
                anchors.centerIn: parent
                visible: root.playingNow
                barWidth: 4
                barHeight: 22
                barSpacing: 4
                playing: visible && enabled
            }
        }

        LiveBadge {
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.margins: 6
            width: implicitWidth
            height: implicitHeight
            visible: root.entry.track.live
            pulsing: root.playingNow
        }
    }

    Column {
        id: details

        anchors.left: cover.right
        anchors.leftMargin: root.compact ? 14 : 18
        anchors.right: actions.left
        anchors.rightMargin: root.compact ? 6 : 12
        y: cover.y
        spacing: 6
        opacity: root.presence

        StyledText {
            width: parent.width
            text: root.entry.title
            title: true
            font.pixelSize: Theme.font.small
            color: root.active ? Theme.colPrimary : root.entry.played ? Theme.colOnSurfaceVariant
                : Theme.colOnSurface
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            elide: Text.ElideRight
        }

        StyledText {
            width: parent.width
            visible: root.entry.description.length > 0
            text: root.entry.description
            textFormat: Text.PlainText
            font.pixelSize: Theme.font.smallie
            color: Theme.colInactive
            wrapMode: Text.WordWrap
            maximumLineCount: root.compact ? 1 : 2
            elide: Text.ElideRight
        }

        Row {
            spacing: 8
            height: 22

            StyledText {
                anchors.verticalCenter: parent.verticalCenter
                visible: root.entry.subtitle.length > 0
                text: root.entry.subtitle
                font.pixelSize: Theme.font.smaller
                color: Theme.colInactive
            }

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 3
                height: 3
                radius: 2
                visible: root.entry.subtitle.length > 0 && (root.remaining.length > 0 || root.entry.played)
                color: Theme.colInactive
            }

            Row {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 5
                visible: root.entry.played

                Sym {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "check_circle"
                    iconSize: 15
                    fill: 1
                    color: Theme.colPrimary
                }

                StyledText {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Played")
                    title: true
                    font.pixelSize: Theme.font.smaller
                    color: Theme.colPrimary
                }
            }

            StyledText {
                anchors.verticalCenter: parent.verticalCenter
                visible: !root.entry.played && root.remaining.length > 0
                text: root.remaining
                title: root.started
                font.pixelSize: Theme.font.smaller
                color: root.started ? Theme.colOnSurfaceVariant : Theme.colInactive
            }

            Item {
                anchors.verticalCenter: parent.verticalCenter
                width: (root.compact ? 56 : 96) * root.startedReveal
                height: 4
                visible: root.startedReveal > 0
                opacity: root.startedReveal

                Rectangle {
                    anchors.fill: parent
                    radius: 2
                    color: Theme.colSecondaryContainer
                }

                Rectangle {
                    width: parent.width * Math.max(0, Math.min(1, root.progress))
                    height: parent.height
                    radius: 2
                    color: Theme.colPrimary
                }
            }
        }
    }

    Row {
        id: actions

        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2

        Sym {
            anchors.verticalCenter: parent.verticalCenter
            opacity: root.downloaded ? 1 : 0
            text: "download_done"
            iconSize: Theme.font.normal
            color: Theme.colPrimary
            Accessible.ignored: !root.downloaded
            Accessible.name: qsTr("Downloaded")
        }

        IconButton {
            anchors.verticalCenter: parent.verticalCenter
            visible: root.writable && root.entry.actions.laterable
            opacity: root.entry.actions.later || root.hovered ? 1 : 0
            interactive: opacity > 0
            icon: root.entry.actions.later ? "playlist_add_check" : "playlist_add"
            toggled: root.entry.actions.later
            iconSize: Theme.font.larger
            diameter: 38
            Accessible.name: root.entry.actions.later ? qsTr("Remove from Episodes for Later")
                : qsTr("Queue to Episodes for Later")
            onClicked: LibraryActions.setQueuedForLater(root.entry.track.videoId, !root.entry.actions.later)

            Behavior on opacity {
                NumberAnimation {
                    duration: Theme.duration.fast
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Theme.curve.expressiveEffects
                }
            }
        }

        IconButton {
            id: overflow

            anchors.verticalCenter: parent.verticalCenter
            icon: "more_vert"
            iconSize: Theme.font.normal
            diameter: 38
            opacity: root.hovered ? 1 : 0
            interactive: root.hovered
            Accessible.name: qsTr("More actions")
            onClicked: {
                const spot = root.mapFromItem(overflow, overflow.width / 2, overflow.height);
                root.menuRequested(spot.x, spot.y);
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

    onClicked: root.playRequested()
    onRightClicked: (x, y) => root.menuRequested(x, y)
}
