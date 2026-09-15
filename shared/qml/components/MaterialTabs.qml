import QtQuick
import HtMusic

Item {
    id: root

    property var labels: []
    property int current: 0
    property real padding: 20
    property real position: root.current

    readonly property int count: root.labels.length
    readonly property Item lower: tabs.itemAt(Math.floor(root.position))
    readonly property Item upper: tabs.itemAt(Math.ceil(root.position))
    readonly property real blend: root.position - Math.floor(root.position)
    readonly property bool measured: root.lower !== null && root.upper !== null

    implicitHeight: 48

    Behavior on position {
        NumberAnimation {
            duration: Theme.duration.resize
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.emphasized
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: Theme.colOutlineVariant
    }

    Row {
        id: strip

        height: parent.height

        Repeater {
            id: tabs

            model: root.labels

            delegate: RippleSurface {
                id: tab

                required property string modelData
                required property int index

                readonly property bool selected: root.current === tab.index

                width: label.implicitWidth + root.padding * 2
                height: strip.height
                rounding: Theme.rounding.verysmall
                colState: tab.selected ? Theme.colPrimary : Theme.colOnSurface
                Accessible.role: Accessible.PageTab
                Accessible.name: tab.modelData

                StyledText {
                    id: label

                    anchors.centerIn: parent
                    text: tab.modelData
                    title: tab.selected
                    font.pixelSize: Theme.font.smallie
                    color: tab.selected ? Theme.colPrimary : Theme.colOnSurfaceVariant
                }

                onClicked: root.current = tab.index
            }
        }
    }

    Rectangle {
        x: root.measured
            ? root.lower.x + root.padding
                + (root.upper.x - root.lower.x) * root.blend
            : 0
        width: root.measured
            ? root.lower.width - root.padding * 2
                + (root.upper.width - root.lower.width) * root.blend
            : 0
        anchors.bottom: parent.bottom
        height: 3
        radius: height / 2
        color: Theme.colPrimary
        visible: root.count > 1
    }
}
