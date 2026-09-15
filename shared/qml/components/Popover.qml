import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import HtMusic

Popup {
    id: root

    property real rounding: Theme.rounding.normal
    property real inset: 6
    property real gap: 2
    property real anchorX: 0.5
    property real anchorY: 0

    readonly property real columnWidth: root.width - root.inset * 2

    default property alias content: column.data

    function toggle() {
        if (root.opened)
            root.close();
        else
            root.open();
    }

    height: plate.implicitHeight
    padding: 0
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

    background: null

    contentItem: Item {
        id: body

        property real reveal: 0

        readonly property real growth: 0.94 + 0.06 * body.reveal

        opacity: Math.min(1, body.reveal * 2.4)

        transform: Scale {
            origin.x: body.width * root.anchorX
            origin.y: plate.implicitHeight * root.anchorY
            xScale: body.growth
            yScale: body.growth
        }

        RectangularShadow {
            anchors.fill: plate
            radius: plate.radius
            blur: 28
            spread: 1
            offset: Qt.vector2d(0, 4)
            color: ColorUtils.withAlpha(Theme.colShadow, 0.34)
        }

        Rectangle {
            id: plate

            y: (plate.implicitHeight - plate.height) * root.anchorY
            width: parent.width
            implicitHeight: column.implicitHeight + root.inset * 2
            height: implicitHeight * (0.42 + 0.58 * body.reveal)
            radius: root.rounding
            color: Theme.colLayer3
            clip: true

            Column {
                id: column

                x: root.inset
                y: root.inset - plate.y
                width: root.columnWidth
                spacing: root.gap
            }
        }
    }

    enter: Transition {
        NumberAnimation {
            target: body
            property: "reveal"
            from: 0
            to: 1
            duration: Theme.duration.spatialFast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveFastSpatial
        }
    }

    exit: Transition {
        NumberAnimation {
            target: body
            property: "reveal"
            from: 1
            to: 0
            duration: Theme.duration.exit
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.emphasizedAccel
        }
    }
}
