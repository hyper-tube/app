import QtQuick
import QtQuick.Controls
import HtMusic

ToolTip {
    id: root

    property real reveal: 0

    readonly property real unseen: 0.001

    opacity: reveal
    scale: 0.9 + 0.1 * reveal
    delay: 550
    timeout: -1
    padding: 10
    horizontalPadding: 14
    margins: 8
    y: parent.height + 8
    z: 200

    contentItem: StyledText {
        text: root.text
        color: Theme.colInverseOnSurface
        font.pixelSize: Theme.font.smaller
    }

    background: Rectangle {
        radius: Theme.rounding.verysmall
        color: Theme.colInverseSurface
    }

    onAboutToShow: root.reveal = root.unseen

    enter: Transition {
        NumberAnimation {
            target: root
            property: "reveal"
            from: 0
            to: 1
            duration: Theme.duration.enter
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.emphasizedDecel
        }
    }

    exit: Transition {
        NumberAnimation {
            target: root
            property: "reveal"
            to: 0
            duration: Theme.duration.exit
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.emphasizedAccel
        }
    }
}
