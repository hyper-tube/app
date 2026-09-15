import QtQuick
import HtMusic

Item {
    id: root

    property bool active: false
    property string label: ""
    property real progress: 0
    property real reveal: root.active ? 1 : 0

    height: 86 * root.reveal
    visible: root.reveal > 0

    Behavior on reveal {
        NumberAnimation {
            duration: Theme.duration.spatialFast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveFastSpatial
        }
    }

    Rectangle {
        width: parent.width
        height: 76
        y: 16 * (1 - root.reveal)
        opacity: Math.min(1, root.reveal)
        radius: Theme.rounding.normal
        color: Theme.colLayer4

        Column {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 10

            StyledText {
                width: parent.width
                text: root.label
                elide: Text.ElideRight
                font.pixelSize: Theme.font.smallie
            }

            Rectangle {
                width: parent.width
                height: 4
                radius: 2
                color: Theme.colOutlineVariant

                Rectangle {
                    width: parent.width * Math.max(0, Math.min(1, root.progress))
                    height: parent.height
                    radius: 2
                    color: Theme.colPrimary

                    Behavior on width {
                        NumberAnimation { duration: Theme.duration.fast; easing.type: Easing.OutCubic }
                    }
                }
            }
        }
    }
}
