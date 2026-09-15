import QtQuick
import HtMusic

Rectangle {
    id: root

    property bool filled: true
    property bool pulsing: true
    property real pixelSize: Theme.font.smallest
    property real pulse: 0

    readonly property color colContent: root.filled ? Theme.colOnError : Theme.colError

    implicitWidth: label.implicitWidth + beacon.width + row.spacing + root.implicitHeight
    implicitHeight: Math.round(root.pixelSize * 1.9)
    radius: Theme.rounding.full
    color: root.filled ? Theme.colError : ColorUtils.withAlpha(Theme.colError, 0.16)
    Accessible.role: Accessible.StaticText
    Accessible.name: qsTr("Live")

    Row {
        id: row

        anchors.centerIn: parent
        spacing: Math.round(root.pixelSize * 0.45)

        Item {
            id: beacon

            anchors.verticalCenter: parent.verticalCenter
            width: Math.round(root.pixelSize * 0.55)
            height: beacon.width

            Rectangle {
                anchors.fill: parent
                radius: Theme.rounding.full
                color: root.colContent
                opacity: (1 - root.pulse) * 0.55
                scale: 1 + root.pulse * 1.3
            }

            Rectangle {
                anchors.fill: parent
                radius: Theme.rounding.full
                color: root.colContent
            }
        }

        StyledText {
            id: label

            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("LIVE")
            title: true
            font.pixelSize: root.pixelSize
            font.letterSpacing: root.pixelSize * 0.06
            color: root.colContent
        }
    }

    NumberAnimation on pulse {
        running: root.pulsing && root.visible && root.opacity > 0
        loops: Animation.Infinite
        from: 0
        to: 1
        duration: 1600
        easing.type: Easing.BezierSpline
        easing.bezierCurve: Theme.curve.standardDecel
    }
}
