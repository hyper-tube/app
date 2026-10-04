import QtQuick
import HtMusic

WheelHandler {
    id: root

    property real destination: 0

    readonly property real pixelsPerLine: 100 / 3

    readonly property NumberAnimation glide: NumberAnimation {
        target: root.target
        property: "contentY"
        duration: Theme.duration.spatialFast
        easing.type: Easing.BezierSpline
        easing.bezierCurve: Theme.curve.emphasizedDecel
    }

    readonly property Connections interruption: Connections {
        target: root.target

        function onMovementStarted() {
            root.glide.stop();
        }
    }

    function bounded(value) {
        const minimum = root.target.originY;
        const maximum = minimum + Math.max(0, root.target.contentHeight - root.target.height);
        return Math.max(minimum, Math.min(maximum, value));
    }

    acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad

    onWheel: event => {
        const precise = event.pixelDelta.y !== 0;
        const delta = precise
            ? event.pixelDelta.y
            : event.angleDelta.y / 120 * Qt.styleHints.wheelScrollLines * root.pixelsPerLine;
        if (delta === 0 || (event.modifiers & Qt.ShiftModifier)) {
            event.accepted = false;
            return;
        }

        event.accepted = true;
        root.target.cancelFlick();
        if (precise) {
            root.glide.stop();
            root.target.contentY = root.bounded(root.target.contentY - delta);
            return;
        }

        const origin = root.glide.running ? root.destination : root.target.contentY;
        root.destination = root.bounded(origin - delta);
        root.glide.stop();
        if (root.destination === root.target.contentY)
            return;
        root.glide.from = root.target.contentY;
        root.glide.to = root.destination;
        root.glide.start();
    }
}
