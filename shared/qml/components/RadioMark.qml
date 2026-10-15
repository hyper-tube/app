import QtQuick
import HtMusic

Item {
    id: root

    property bool checked: false
    property real progress: checked ? 1 : 0

    readonly property real ring: Theme.sized(20)

    implicitWidth: Theme.sized(22)
    implicitHeight: Theme.sized(22)

    Rectangle {
        anchors.centerIn: parent
        width: root.ring
        height: root.ring
        radius: root.ring / 2
        color: "transparent"
        border.width: 2
        border.color: ColorUtils.mix(Theme.colPrimary, Theme.colOutline, root.progress)
    }

    Rectangle {
        anchors.centerIn: parent
        width: root.ring * 0.5 * root.progress
        height: width
        radius: width / 2
        color: Theme.colPrimary
    }

    Behavior on progress {
        NumberAnimation {
            duration: Theme.duration.spatialFast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveFastSpatial
        }
    }
}
