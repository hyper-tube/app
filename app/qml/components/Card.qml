import QtQuick
import HtMusic

RippleSurface {
    id: root

    property string title: ""
    property string subtitle: ""
    property string artId: ""
    property bool circular: false
    property bool video: false
    property bool playable: false
    property bool pinned: false
    property bool live: false
    property bool active: false
    property real artSize: 168

    readonly property real inset: Theme.size.cardInset
    readonly property bool playingNow: root.active && PlaybackController.playing
    readonly property bool cueVisible: root.hovered && !root.playingNow

    signal playRequested
    signal menuRequested(real x, real y)

    implicitWidth: root.artSize * (root.video ? 16 / 9 : 1) + root.inset * 2
    implicitHeight: root.artSize + root.inset * 2 + labels.implicitHeight + 12
    rounding: Theme.rounding.normal

    Item {
        id: cover

        x: root.inset
        y: root.inset
        width: root.artSize * (root.video ? 16 / 9 : 1)
        height: root.artSize
        scale: root.hovered ? 1.03 : 1

        Behavior on scale {
            NumberAnimation {
                duration: Theme.duration.fast
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveEffects
            }
        }

        Artwork {
            id: art

            anchors.fill: parent
            artId: root.artId
            rounding: root.circular ? Theme.rounding.full : Theme.rounding.small
        }

        Rectangle {
            anchors.fill: parent
            anchors.margins: -1
            radius: art.rounding + 1
            color: ColorUtils.withAlpha(Theme.colScrim, 0.42)
            opacity: root.hovered ? 1 : root.active ? 0.5 : 0

            Behavior on opacity {
                NumberAnimation {
                    duration: Theme.duration.fast
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Theme.curve.expressiveEffects
                }
            }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.margins: 8
            width: 30
            height: 30
            radius: Theme.rounding.full
            color: ColorUtils.withAlpha(Theme.colLayer1, 0.85)
            visible: root.pinned

            Sym {
                anchors.centerIn: parent
                text: "keep"
                iconSize: Theme.font.normal
                fill: 1
                color: Theme.colPrimary
            }
        }

        LiveBadge {
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.margins: 8
            width: implicitWidth
            height: implicitHeight
            visible: root.live
            pixelSize: Theme.font.smallest
        }

        Rectangle {
            anchors.centerIn: parent
            width: 54
            height: 54
            radius: Theme.rounding.full
            color: ColorUtils.withAlpha(Theme.colLayer1, 0.85)
            visible: root.active && !root.cueVisible

            PlayingIndicator {
                anchors.centerIn: parent
                barWidth: 4
                barHeight: 24
                barSpacing: 4
                playing: root.playingNow
            }
        }

        IconButton {
            id: overflow

            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 6
            icon: "more_vert"
            iconSize: 20
            diameter: 34
            colBackground: ColorUtils.withAlpha(Theme.colLayer1, 0.85)
            colIcon: Theme.colOnSurfaceVariant
            opacity: root.hovered ? 1 : 0
            interactive: root.hovered
            Accessible.name: qsTr("More actions")
            onClicked: {
                const spot = root.mapFromItem(overflow, overflow.width, overflow.height);
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

        IconButton {
            x: root.playable || root.circular ? (parent.width - width) / 2 : parent.width - width - 10
            y: root.playable || root.circular ? (parent.height - height) / 2 : parent.height - height - 10
            icon: "play_arrow"
            Accessible.name: qsTr("Play")
            iconSize: 24
            diameter: 44
            filled: true
            opacity: root.cueVisible ? 1 : 0
            scale: root.cueVisible ? 1 : 0.7
            interactive: root.cueVisible
            onClicked: root.playRequested()

            Behavior on opacity {
                NumberAnimation {
                    duration: Theme.duration.fast
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Theme.curve.expressiveEffects
                }
            }

            Behavior on scale {
                NumberAnimation {
                    duration: Theme.duration.fast
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Theme.curve.expressiveFastSpatial
                }
            }
        }
    }

    Column {
        id: labels

        x: root.inset
        y: cover.y + cover.height + 12
        width: root.artSize * (root.video ? 16 / 9 : 1)
        spacing: 2

        StyledText {
            width: parent.width
            text: root.title
            title: true
            horizontalAlignment: root.circular ? Text.AlignHCenter : Text.AlignLeft
            font.pixelSize: Theme.font.small
            color: root.active ? Theme.colPrimary : Theme.colOnSurface
            elide: Text.ElideRight
        }

        StyledText {
            width: parent.width
            visible: root.subtitle.length > 0
            text: root.subtitle
            horizontalAlignment: root.circular ? Text.AlignHCenter : Text.AlignLeft
            font.pixelSize: Theme.font.smaller
            color: Theme.colInactive
            elide: Text.ElideRight
        }
    }

    onRightClicked: (x, y) => root.menuRequested(x, y)
}
