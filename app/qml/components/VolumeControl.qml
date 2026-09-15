import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import HtMusic

Item {
    id: root

    property bool compact: false

    readonly property int level: PlaybackController.muted || PlaybackController.volume === 0
        ? 0 : PlaybackController.volume < 50 ? 1 : 2
    readonly property string icon: ["volume_off", "volume_down", "volume_up"][root.level]
    readonly property real glyphSize: [21, 26, 23][root.level]
    readonly property real approach: 10
    readonly property bool engaged: root.compact && (button.hovered || trayHover.hovered || vertical.dragging)

    implicitWidth: root.compact ? button.width : button.width + 6 + inline.width
    implicitHeight: 40

    IconButton {
        id: button

        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        icon: root.icon
        iconSize: root.glyphSize
        toggled: PlaybackController.muted
        Accessible.name: PlaybackController.muted ? qsTr("Unmute") : qsTr("Mute")
        onClicked: PlaybackController.toggleMuted()
    }

    SeekBar {
        id: inline

        anchors.left: button.right
        anchors.leftMargin: 6
        anchors.verticalCenter: parent.verticalCenter
        visible: !root.compact
        width: 84
        trackHeight: 4
        activeTrackHeight: 6
        continuous: true
        position: PlaybackController.muted ? 0 : PlaybackController.volume
        duration: 100
        onSeeked: level => {
            PlaybackController.muted = false;
            PlaybackController.volume = level;
        }
    }

    Popup {
        id: panel

        property real reveal: 0

        parent: button
        x: (button.width - width) / 2
        y: -height
        width: 48
        height: 168
        padding: 0
        focus: false
        visible: root.engaged
        closePolicy: Popup.NoAutoClose

        background: null

        contentItem: Item {
            id: tray

            opacity: panel.reveal
            scale: 0.9 + 0.1 * panel.reveal
            transformOrigin: Item.Bottom

            RectangularShadow {
                anchors.fill: plate
                radius: plate.radius
                blur: 24
                spread: 1
                offset: Qt.vector2d(0, 3)
                color: ColorUtils.withAlpha(Theme.colShadow, 0.34)
            }

            Rectangle {
                id: plate

                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                height: parent.height - root.approach
                radius: Theme.rounding.normal
                color: Theme.colLayer3

                SeekBar {
                    id: vertical

                    anchors.centerIn: parent
                    width: plate.height - 34
                    rotation: -90
                    trackHeight: 5
                    activeTrackHeight: 7
                    continuous: true
                    position: PlaybackController.muted ? 0 : PlaybackController.volume
                    duration: 100
                    onSeeked: level => {
                        PlaybackController.muted = false;
                        PlaybackController.volume = level;
                    }
                }
            }

            HoverHandler {
                id: trayHover
            }
        }

        enter: Transition {
            NumberAnimation {
                target: panel
                property: "reveal"
                from: 0
                to: 1
                duration: Theme.duration.fast
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveEffects
            }
        }

        exit: Transition {
            NumberAnimation {
                target: panel
                property: "reveal"
                from: 1
                to: 0
                duration: Theme.duration.exit
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.emphasizedAccel
            }
        }
    }
}
