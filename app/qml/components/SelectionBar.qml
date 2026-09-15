import QtQuick
import QtQuick.Effects
import HtMusic

Item {
    id: root

    property bool active: false
    property int count: 0
    property bool removable: false

    property real reveal: root.active ? 1 : 0

    signal cancelRequested
    signal saveRequested
    signal removeRequested
    signal playNextRequested
    signal queueRequested
    signal downloadRequested

    visible: card.opacity > 0
    implicitWidth: card.width
    implicitHeight: card.height

    Behavior on reveal {
        NumberAnimation {
            duration: Theme.duration.spatialFast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveFastSpatial
        }
    }

    RectangularShadow {
        anchors.fill: card
        radius: card.radius
        blur: 32
        spread: 1
        offset: Qt.vector2d(0, 6)
        opacity: card.opacity
        color: ColorUtils.withAlpha(Theme.colShadow, 0.4)
    }

    Rectangle {
        id: card

        width: controls.implicitWidth + 24
        height: 64
        radius: Theme.rounding.full
        color: Theme.colLayer4
        opacity: Math.min(1, root.reveal * 1.6)

        transform: Translate { y: (1 - root.reveal) * 28 }

        Row {
            id: controls

            anchors.centerIn: parent
            spacing: 4

            IconButton {
                anchors.verticalCenter: parent.verticalCenter
                icon: "close"
                Accessible.name: qsTr("Cancel selection")
                onClicked: root.cancelRequested()
            }

            StyledText {
                anchors.verticalCenter: parent.verticalCenter
                width: 96
                text: qsTr("%n selected", "", root.count)
                title: true
                font.pixelSize: Theme.font.smallie
                color: Theme.colOnSurface
            }

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 1
                height: 26
                color: Theme.colOutlineVariant
            }

            IconButton {
                anchors.verticalCenter: parent.verticalCenter
                icon: "playlist_play"
                Accessible.name: qsTr("Play selected next")
                interactive: root.count > 0
                opacity: interactive ? 1 : 0.35
                onClicked: root.playNextRequested()
            }

            IconButton {
                anchors.verticalCenter: parent.verticalCenter
                icon: "queue_music"
                Accessible.name: qsTr("Add selected to queue")
                interactive: root.count > 0
                opacity: interactive ? 1 : 0.35
                onClicked: root.queueRequested()
            }

            IconButton {
                anchors.verticalCenter: parent.verticalCenter
                icon: "playlist_add"
                Accessible.name: qsTr("Save selected to playlist")
                interactive: root.count > 0 && Account.signedIn
                opacity: interactive ? 1 : 0.35
                onClicked: root.saveRequested()
            }

            IconButton {
                anchors.verticalCenter: parent.verticalCenter
                icon: "download"
                Accessible.name: qsTr("Download selected")
                interactive: root.count > 0
                opacity: interactive ? 1 : 0.35
                onClicked: root.downloadRequested()
            }

            IconButton {
                anchors.verticalCenter: parent.verticalCenter
                visible: root.removable
                icon: "playlist_remove"
                colIcon: Theme.colError
                colState: Theme.colError
                Accessible.name: qsTr("Remove selected from playlist")
                interactive: root.count > 0
                opacity: interactive ? 1 : 0.35
                onClicked: root.removeRequested()
            }
        }
    }
}
