import QtQuick
import HtMusic

Item {
    id: root

    property real hue: 0
    property real saturation: 1
    property real brightness: 1

    readonly property color value: Qt.hsva(root.hue, root.saturation, root.brightness, 1)

    signal picked(color chosen)

    function adopt(source) {
        const chosen = Qt.color(source);
        if (!chosen.valid)
            return;
        root.hue = Math.max(0, chosen.hsvHue);
        root.saturation = chosen.hsvSaturation;
        root.brightness = chosen.hsvValue;
    }

    function commit() {
        root.clearFocus();
        root.picked(root.value);
    }

    function clearFocus() {
        hexField.text = "";
        hexField.clearFocus();
    }

    implicitHeight: field.height + strip.height + hex.height + 20

    Rectangle {
        id: field

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 128
        radius: Theme.rounding.small
        color: Qt.hsva(root.hue, 1, 1, 1)

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "#ffffffff" }
                GradientStop { position: 1.0; color: "#00ffffff" }
            }
        }

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#00000000" }
                GradientStop { position: 1.0; color: "#ff000000" }
            }
        }

        Rectangle {
            x: root.saturation * parent.width - width / 2
            y: (1 - root.brightness) * parent.height - height / 2
            width: 16
            height: 16
            radius: Theme.rounding.full
            color: "transparent"
            border.width: 3
            border.color: "#ffffff"
        }

        HoverHandler {
            cursorShape: Qt.CrossCursor
        }

        TapHandler {
            gesturePolicy: TapHandler.ReleaseWithinBounds
            onSingleTapped: eventPoint => {
                root.saturation = Math.max(0, Math.min(1, eventPoint.position.x / field.width));
                root.brightness = 1 - Math.max(0, Math.min(1, eventPoint.position.y / field.height));
                root.commit();
            }
        }

        DragHandler {
            target: null

            onCentroidChanged: {
                if (!active)
                    return;
                root.saturation = Math.max(0, Math.min(1, centroid.position.x / field.width));
                root.brightness = 1 - Math.max(0, Math.min(1, centroid.position.y / field.height));
                root.commit();
            }
        }
    }

    Rectangle {
        id: strip

        anchors.top: field.bottom
        anchors.topMargin: 10
        anchors.left: parent.left
        anchors.right: parent.right
        height: 18
        radius: Theme.rounding.full

        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.000; color: "#ff0000" }
            GradientStop { position: 0.167; color: "#ffff00" }
            GradientStop { position: 0.333; color: "#00ff00" }
            GradientStop { position: 0.500; color: "#00ffff" }
            GradientStop { position: 0.667; color: "#0000ff" }
            GradientStop { position: 0.833; color: "#ff00ff" }
            GradientStop { position: 1.000; color: "#ff0000" }
        }

        Rectangle {
            x: root.hue * parent.width - width / 2
            anchors.verticalCenter: parent.verticalCenter
            width: 10
            height: parent.height + 8
            radius: Theme.rounding.full
            color: "transparent"
            border.width: 3
            border.color: "#ffffff"
        }

        HoverHandler {
            cursorShape: Qt.PointingHandCursor
        }

        TapHandler {
            gesturePolicy: TapHandler.ReleaseWithinBounds
            onSingleTapped: eventPoint => {
                root.hue = Math.max(0, Math.min(0.9999, eventPoint.position.x / strip.width));
                root.commit();
            }
        }

        DragHandler {
            target: null
            yAxis.enabled: false

            onCentroidChanged: {
                if (!active)
                    return;
                root.hue = Math.max(0, Math.min(0.9999, centroid.position.x / strip.width));
                root.commit();
            }
        }
    }

    Item {
        id: hex

        anchors.top: strip.bottom
        anchors.topMargin: 10
        anchors.left: parent.left
        anchors.right: parent.right
        height: 42

        Rectangle {
            id: preview

            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: 42
            height: 42
            radius: Theme.rounding.full
            color: root.value
        }

        SearchBar {
            id: hexField

            anchors.left: preview.right
            anchors.leftMargin: 10
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            icon: ""
            placeholder: root.value.toString()
            onAccepted: entered => {
                const chosen = Qt.color(entered.trim());
                if (!chosen.valid)
                    return;
                root.adopt(chosen);
                root.commit();
            }
        }
    }
}
