import QtQuick
import HtMusic

Item {
    id: root

    required property Flickable target
    property real step: 0
    property real position: 0

    readonly property real give: 56
    readonly property real minimumX: root.target.originX
    readonly property real maximumX: root.minimumX
        + Math.max(0, root.target.contentWidth - root.target.width)

    function scrollBy(direction) {
        glide.stop();
        settle.stop();
        root.position = root.bounded(root.target.contentX);
        glide.from = root.position;
        glide.to = Math.max(root.minimumX - root.give,
            Math.min(root.maximumX + root.give, root.position + root.step * direction));
        glide.start();
    }

    function bounded(value) {
        return Math.max(root.minimumX, Math.min(root.maximumX, value));
    }

    function stretched(value) {
        const beyond = value - root.maximumX;
        if (beyond > 0)
            return root.maximumX + root.give * beyond / (beyond + root.give);
        const before = root.minimumX - value;
        if (before > 0)
            return root.minimumX - root.give * before / (before + root.give);
        return value;
    }

    visible: false

    onPositionChanged: root.target.contentX = root.stretched(root.position)

    NumberAnimation {
        id: glide

        target: root
        property: "position"
        duration: Theme.duration.spatial
        easing.type: Easing.BezierSpline
        easing.bezierCurve: Theme.curve.emphasized

        onFinished: {
            settle.from = root.target.contentX;
            settle.to = root.bounded(settle.from);
            if (settle.to !== settle.from)
                settle.start();
        }
    }

    NumberAnimation {
        id: settle

        target: root.target
        property: "contentX"
        duration: Theme.duration.spatialFast
        easing.type: Easing.BezierSpline
        easing.bezierCurve: Theme.curve.emphasizedDecel

        onFinished: root.target.returnToBounds()
    }
}
