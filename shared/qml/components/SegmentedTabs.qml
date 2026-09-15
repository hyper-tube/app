import QtQuick
import HtMusic

Item {
    id: root

    property var labels: []
    property int current: 0
    property bool glass: false
    property real inset: 4

    readonly property int count: root.labels.length
    readonly property real segmentWidth: root.count > 0
        ? (root.width - root.inset * 2) / root.count
        : 0
    readonly property real segmentHeight: root.height - root.inset * 2

    implicitWidth: Math.max(160, root.count * 116)
    implicitHeight: 42

    Rectangle {
        anchors.fill: parent
        radius: Theme.rounding.full
        color: ColorUtils.withAlpha(Theme.colLayer3, root.glass ? 0.4 : 0.75)
    }

    Rectangle {
        x: root.inset + root.segmentWidth * root.current
        y: root.inset
        width: root.segmentWidth
        height: root.segmentHeight
        radius: Theme.rounding.full
        color: Theme.colSecondaryContainer

        Behavior on x {
            NumberAnimation {
                duration: Theme.duration.spatialFast
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveFastSpatial
            }
        }
    }

    Row {
        x: root.inset
        y: root.inset

        Repeater {
            model: root.labels

            delegate: RippleSurface {
                id: segment

                required property string modelData
                required property int index

                readonly property bool selected: root.current === index

                width: root.segmentWidth
                height: root.segmentHeight
                rounding: Theme.rounding.full
                colState: selected ? Theme.colOnSecondaryContainer : Theme.colOnSurface

                StyledText {
                    anchors.centerIn: parent
                    text: segment.modelData
                    title: segment.selected
                    font.pixelSize: Theme.font.smallie
                    color: segment.selected ? Theme.colOnSecondaryContainer : Theme.colOnSurfaceVariant
                }

                onClicked: root.current = index
            }
        }
    }
}
