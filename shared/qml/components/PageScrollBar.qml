import QtQuick
import QtQuick.Controls
import HtMusic

ScrollBar {
    id: root

    property real reveal: root.hovered || root.pressed ? 1 : root.scrolling ? 0.55 : 0
    property bool scrolling: false

    readonly property real gutter: 14
    readonly property real barWidth: 4 + 4 * root.reveal

    implicitWidth: root.gutter
    topPadding: 6
    bottomPadding: 6
    leftPadding: (root.gutter - root.barWidth) / 2
    rightPadding: (root.gutter - root.barWidth) / 2
    minimumSize: 0.06
    hoverEnabled: true
    visible: root.size > 0 && root.size < 1
    opacity: root.reveal

    onPositionChanged: {
        root.scrolling = true;
        idle.restart();
    }

    background: Item {
        Rectangle {
            anchors.centerIn: parent
            width: root.barWidth
            height: parent.height - root.topPadding - root.bottomPadding
            radius: width / 2
            color: ColorUtils.withAlpha(Theme.colOnSurface, 0.10)
        }
    }

    contentItem: Rectangle {
        radius: width / 2
        color: root.pressed ? Theme.colOnSurface : Theme.colOnSurfaceVariant

        Behavior on color {
            ColorAnimation {
                duration: Theme.duration.fast
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveEffects
            }
        }
    }

    Timer {
        id: idle

        interval: 1200
        onTriggered: root.scrolling = false
    }

    Behavior on reveal {
        NumberAnimation {
            duration: Theme.duration.fast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveEffects
        }
    }
}
