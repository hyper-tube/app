import QtQuick
import HtMusic

Item {
    id: root

    property string artId: ""
    property real rounding: Theme.rounding.small
    property int resolution: {
        const pixels = Math.max(width, height) * Screen.devicePixelRatio;
        return pixels <= 64 ? 64 : pixels <= 128 ? 128 : pixels <= 256 ? 256
            : pixels <= 512 ? 512 : 1024;
    }

    Rectangle {
        anchors.fill: parent
        radius: root.rounding
        color: Theme.colLayer3

        Sym {
            anchors.centerIn: parent
            text: "music_note"
            iconSize: Math.max(16, Math.min(parent.width, parent.height) * 0.34)
            color: Theme.colInactive
        }
    }

    Item {
        id: imageClip

        anchors.fill: parent
        visible: image.status === Image.Ready
        layer.enabled: visible
        layer.effect: RoundedMask {
            rounding: root.rounding
        }

        Image {
            id: image

            anchors.fill: parent
            sourceSize: Qt.size(
                Math.ceil(root.resolution * root.width / Math.max(1, root.width, root.height)),
                Math.ceil(root.resolution * root.height / Math.max(1, root.width, root.height)))

            Binding on source {
                value: root.width > 0 && root.height > 0 && root.artId ? "image://art/" + encodeURIComponent(root.artId) : ""
                delayed: true
            }
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            cache: true
        }
    }

    opacity: image.status === Image.Ready || image.status === Image.Null ? 1 : 0.6

    Behavior on opacity {
        NumberAnimation {
            duration: Theme.duration.fast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveEffects
        }
    }
}
