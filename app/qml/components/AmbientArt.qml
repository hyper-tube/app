import QtQuick
import QtQuick.Effects
import HtMusic

Item {
    id: root

    property string artId: ""
    property bool flipped: false
    property real blend: root.flipped ? 1 : 0

    readonly property string source: root.artId
        ? "image://art/" + encodeURIComponent(root.artId) : ""
    readonly property int canvasWidth: 128
    readonly property int canvasHeight: Math.max(1, Math.round(
        root.canvasWidth * root.height / Math.max(1, root.width)))

    Item {
        id: canvas

        width: root.canvasWidth
        height: root.canvasHeight
        transformOrigin: Item.TopLeft
        scale: root.width / root.canvasWidth
        layer.enabled: true
        layer.smooth: true
        layer.effect: MultiEffect {
            blurEnabled: true
            blur: 1
            blurMax: 16
        }

        Image {
            id: leading

            anchors.fill: parent
            sourceSize.width: 96
            sourceSize.height: 96
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            opacity: 1 - root.blend

            onStatusChanged: if (status === Image.Ready && root.flipped) root.flipped = false
        }

        Image {
            id: trailing

            anchors.fill: parent
            sourceSize.width: 96
            sourceSize.height: 96
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            opacity: root.blend

            onStatusChanged: if (status === Image.Ready && !root.flipped) root.flipped = true
        }
    }

    onSourceChanged: {
        const incoming = root.flipped ? leading : trailing;
        incoming.source = root.source;
        if (!root.source)
            root.flipped = !root.flipped;
    }

    Behavior on blend {
        NumberAnimation {
            duration: Theme.duration.sheetEnter
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveEffects
        }
    }
}
