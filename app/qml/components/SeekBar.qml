import QtQuick
import HtMusic

Item {
    id: root

    property real position: 0
    property real duration: 0
    property real trackHeight: 5
    property real activeTrackHeight: 9
    property bool continuous: false
    property bool live: false

    readonly property real progress: root.live ? 1
        : root.duration > 0
        ? Math.max(0, Math.min(1, (dragging ? dragFraction : root.position) / root.duration))
        : 0
    readonly property bool dragging: drag.active
    readonly property bool engaged: !root.live && (dragging || hover.hovered)

    property real dragFraction: 0

    signal seeked(int value)

    implicitHeight: 22

    function fractionAt(x) {
        return Math.max(0, Math.min(1, x / Math.max(1, root.width)));
    }

    Item {
        id: bar

        anchors.verticalCenter: parent.verticalCenter
        width: parent.width
        height: root.engaged ? root.activeTrackHeight : root.trackHeight

        Behavior on height {
            NumberAnimation {
                duration: Theme.duration.fast
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveEffects
            }
        }

        Rectangle {
            anchors.fill: parent
            radius: Theme.rounding.full
            color: ColorUtils.withAlpha(Theme.colOnSurface, 0.18)
        }

        Rectangle {
            width: Math.max(parent.height, parent.width * root.progress)
            height: parent.height
            radius: Theme.rounding.full
            color: root.live ? Theme.colError : Theme.colPrimary

            Behavior on color {
                ColorAnimation {
                    duration: Theme.duration.fast
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Theme.curve.expressiveEffects
                }
            }
        }
    }

    Rectangle {
        id: handle

        x: root.width * root.progress - width / 2
        anchors.verticalCenter: parent.verticalCenter
        width: root.engaged ? 6 : 0
        height: root.engaged ? 20 : 0
        radius: Theme.rounding.full
        color: Theme.colPrimary

        Behavior on width {
            NumberAnimation {
                duration: Theme.duration.fast
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveEffects
            }
        }

        Behavior on height {
            NumberAnimation {
                duration: Theme.duration.fast
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveEffects
            }
        }
    }

    HoverHandler {
        id: hover

        enabled: !root.live
        cursorShape: Qt.PointingHandCursor
    }

    TapHandler {
        enabled: !root.live
        gesturePolicy: TapHandler.ReleaseWithinBounds

        onSingleTapped: eventPoint => {
            if (root.duration > 0)
                root.seeked(root.fractionAt(eventPoint.position.x) * root.duration);
        }
    }

    DragHandler {
        id: drag

        enabled: !root.live
        target: null

        onCentroidChanged: {
            if (!drag.active)
                return;
            root.dragFraction = root.fractionAt(drag.centroid.position.x) * root.duration;
            if (root.continuous && root.duration > 0)
                root.seeked(root.dragFraction);
        }

        onActiveChanged: {
            if (drag.active) {
                root.dragFraction = root.fractionAt(drag.centroid.position.x) * root.duration;
                if (root.continuous && root.duration > 0)
                    root.seeked(root.dragFraction);
            } else if (root.duration > 0) {
                root.seeked(root.dragFraction);
            }
        }
    }
}
