import QtQuick
import HtMusic

Item {
    id: root

    property bool shown: false
    property real reveal: 0
    property alias text: button.text
    property alias icon: button.icon
    property alias iconSize: button.iconSize
    property alias leadingPadding: button.leadingPadding

    signal clicked

    implicitWidth: button.implicitWidth * root.reveal
    implicitHeight: button.implicitHeight
    visible: root.reveal > 0
    clip: true

    PillButton {
        id: button

        anchors.right: parent.right
        opacity: Math.min(1, root.reveal * 1.5)
        scale: 0.8 + 0.2 * root.reveal
        transformOrigin: Item.Right
        transform: Translate { y: 10 * (1 - root.reveal) }
        interactive: root.shown
        onClicked: root.clicked()
    }

    states: State {
        name: "shown"
        when: root.shown

        PropertyChanges {
            target: root
            reveal: 1
        }
    }

    transitions: [
        Transition {
            to: "shown"

            NumberAnimation {
                property: "reveal"
                duration: Theme.duration.enter
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.emphasizedDecel
            }
        },
        Transition {
            from: "shown"

            NumberAnimation {
                property: "reveal"
                duration: Theme.duration.exit
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.emphasizedAccel
            }
        }
    ]
}
