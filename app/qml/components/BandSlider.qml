import QtQuick
import HtMusic

Item {
    id: root

    property real value: 0
    property real range: 12
    property string label: ""

    readonly property real fraction: 0.5 - root.value / (root.range * 2)
    readonly property real handleY: (track.height - handle.height) * root.fraction
    readonly property bool engaged: drag.active || hover.hovered

    signal moved(real value)

    implicitWidth: 34
    implicitHeight: 170

    function valueAt(y) {
        const span = track.height - handle.height;
        const fraction = Math.max(0, Math.min(1, (y - handle.height / 2) / Math.max(1, span)));
        return Math.round((0.5 - fraction) * root.range * 2);
    }

    Item {
        id: track

        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width
        height: parent.height - 38

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 5
            height: parent.height
            radius: Theme.rounding.full
            color: ColorUtils.withAlpha(Theme.colOnSurface, 0.18)
        }

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            y: Math.min(parent.height / 2, root.handleY + handle.height / 2)
            width: 5
            height: Math.abs(parent.height / 2 - root.handleY - handle.height / 2)
            radius: Theme.rounding.full
            color: Theme.colPrimary
        }

        Rectangle {
            id: handle

            y: root.handleY
            anchors.horizontalCenter: parent.horizontalCenter
            width: root.engaged ? 22 : 18
            height: 8
            radius: Theme.rounding.full
            color: Theme.colPrimary

            Behavior on width {
                NumberAnimation {
                    duration: Theme.duration.fast
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Theme.curve.expressiveEffects
                }
            }
        }

        HoverHandler {
            id: hover

            cursorShape: Qt.PointingHandCursor
        }

        TapHandler {
            gesturePolicy: TapHandler.ReleaseWithinBounds
            onSingleTapped: eventPoint => root.moved(root.valueAt(eventPoint.position.y))
        }

        DragHandler {
            id: drag

            target: null
            xAxis.enabled: false
            yAxis.enabled: true

            onCentroidChanged: {
                if (drag.active)
                    root.moved(root.valueAt(drag.centroid.position.y));
            }
        }
    }

    StyledText {
        anchors.bottom: caption.top
        anchors.bottomMargin: 2
        anchors.horizontalCenter: parent.horizontalCenter
        text: (root.value > 0 ? "+" : "") + root.value
        font.pixelSize: Theme.font.smallest
        color: root.value === 0 ? Theme.colInactive : Theme.colPrimary
    }

    StyledText {
        id: caption

        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        text: root.label
        font.pixelSize: Theme.font.smallest
        color: Theme.colOnSurfaceVariant
    }
}
