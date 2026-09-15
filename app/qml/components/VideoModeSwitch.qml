import QtQuick
import HtMusic

Item {
    id: root

    property bool video: false
    property real inset: 4

    readonly property real segment: (root.width - root.inset * 2) / 2
    readonly property real lane: root.height - root.inset * 2

    signal picked(bool wantsVideo)

    implicitWidth: 80
    implicitHeight: 38

    property real reveal: root.video ? 1 : 0

    Behavior on reveal {
        NumberAnimation {
            duration: Theme.duration.spatialFast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveFastSpatial
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: Theme.rounding.full
        color: ColorUtils.withAlpha(Theme.colLayer1, 0.82)
        border.width: 1
        border.color: ColorUtils.withAlpha(Theme.colOutline, 0.3)
    }

    Rectangle {
        x: root.inset + root.segment * root.reveal
        y: root.inset
        width: root.segment
        height: root.lane
        radius: Theme.rounding.full
        color: Theme.colPrimary
    }

    Row {
        x: root.inset
        y: root.inset

        RippleSurface {
            width: root.segment
            height: root.lane
            rounding: Theme.rounding.full
            colState: root.video ? Theme.colOnSurface : Theme.colOnPrimary
            tooltip: Accessible.name
            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Show the artwork")

            Sym {
                anchors.centerIn: parent
                text: "image"
                iconSize: Theme.font.large
                fill: root.video ? 0 : 1
                color: root.video ? Theme.colOnSurfaceVariant : Theme.colOnPrimary
            }

            onClicked: root.picked(false)
        }

        RippleSurface {
            width: root.segment
            height: root.lane
            rounding: Theme.rounding.full
            colState: root.video ? Theme.colOnPrimary : Theme.colOnSurface
            tooltip: Accessible.name
            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Play the music video")

            Sym {
                anchors.centerIn: parent
                text: "movie"
                iconSize: Theme.font.large
                fill: root.video ? 1 : 0
                color: root.video ? Theme.colOnPrimary : Theme.colOnSurfaceVariant
            }

            onClicked: root.picked(true)
        }
    }
}
