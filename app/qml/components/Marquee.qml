import QtQuick
import HtMusic

Item {
    id: root

    property string text: ""
    property bool title: false
    property real pixelSize: Theme.font.small
    property color color: Theme.colOnSurface
    property bool active: false
    property real gap: 48

    readonly property bool overflowing: label.implicitWidth > root.width + 0.5

    implicitHeight: label.implicitHeight
    clip: true

    Row {
        id: track

        spacing: root.gap

        StyledText {
            id: label

            text: root.text
            title: root.title
            font.pixelSize: root.pixelSize
            color: root.color
            elide: Text.ElideNone
        }

        StyledText {
            visible: scroll.running
            elide: Text.ElideNone
            text: root.text
            title: root.title
            font.pixelSize: root.pixelSize
            color: root.color
        }
    }

    Timer {
        id: reset

        interval: 0
        onTriggered: track.x = 0
    }

    onTextChanged: reset.restart()
    onWidthChanged: reset.restart()
    onPixelSizeChanged: reset.restart()
    onTitleChanged: reset.restart()
    onGapChanged: reset.restart()
    onOverflowingChanged: if (!root.overflowing) track.x = 0
    onActiveChanged: if (!root.active) track.x = 0

    SequentialAnimation {
        id: scroll

        running: root.overflowing && root.active && !reset.running
        loops: Animation.Infinite

        PropertyAction { target: track; property: "x"; value: 0 }
        PauseAnimation { duration: 1800 }
        NumberAnimation {
            target: track
            property: "x"
            from: 0
            to: -(label.implicitWidth + root.gap)
            duration: Math.max(2600, (label.implicitWidth + root.gap) * 16)
            easing.type: Easing.Linear
        }
        PauseAnimation { duration: 400 }
    }
}
