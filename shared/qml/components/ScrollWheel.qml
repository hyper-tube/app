import QtQuick

WheelHandler {
    id: root

    acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad

    onWheel: event => {
        const delta = event.pixelDelta.y !== 0
            ? event.pixelDelta.y
            : event.angleDelta.y / 120 * Qt.styleHints.wheelScrollLines * 20;
        if (delta === 0 || (event.modifiers & Qt.ShiftModifier)) {
            event.accepted = false;
            return;
        }

        const minimum = root.target.originY;
        const maximum = minimum + Math.max(0, root.target.contentHeight - root.target.height);
        const position = Math.max(minimum, Math.min(maximum, root.target.contentY - delta));
        event.accepted = true;
        if (position !== root.target.contentY) {
            root.target.cancelFlick();
            root.target.contentY = position;
        }
    }
}
