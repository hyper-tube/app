import QtQuick
import HtMusic

Item {
    id: root

    property bool shown: false
    property real reveal: 0
    property real gap: 0
    property alias choices: select.choices
    property alias current: select.current
    property alias heading: select.heading
    property alias interactive: select.interactive

    readonly property real presence: Math.max(0, Math.min(1, root.reveal))
    readonly property real contentPresence: Math.max(0, (root.presence - 0.3) / 0.7)

    signal picked(int index)

    implicitWidth: (select.implicitWidth + root.gap) * root.presence
    implicitHeight: select.implicitHeight
    visible: root.reveal > 0

    ChipSelect {
        id: select

        anchors.right: parent.right
        width: implicitWidth
        alignRight: true
        opacity: root.contentPresence
        scale: 0.85 + 0.15 * root.presence
        transformOrigin: Item.Right
        onPicked: index => root.picked(index)
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
                duration: Theme.duration.spatial
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.emphasized
            }
        },
        Transition {
            from: "shown"

            NumberAnimation {
                property: "reveal"
                duration: Theme.duration.spatialFast
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.emphasizedAccel
            }
        }
    ]
}
