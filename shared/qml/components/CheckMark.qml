import QtQuick
import QtQuick.Shapes
import HtMusic

Rectangle {
    id: root

    property bool checked: false
    property real progress: checked ? 1 : 0

    readonly property real fillProgress: Math.min(1, progress * 2.5)
    readonly property real strokeProgress: Math.max(0, (progress - 0.3) / 0.7)
    readonly property real firstStroke: Math.min(1, strokeProgress * 3)
    readonly property real secondStroke: Math.max(0, (strokeProgress - 1 / 3) * 1.5)

    implicitWidth: 22
    implicitHeight: 22
    radius: Theme.rounding.unsharpenmore
    color: "transparent"
    border.width: 2
    border.color: ColorUtils.mix(Theme.colPrimary, Theme.colOutline, root.fillProgress)

    Rectangle {
        anchors.centerIn: parent
        width: root.width * root.fillProgress
        height: root.height * root.fillProgress
        radius: root.radius
        color: Theme.colPrimary
    }

    Shape {
        anchors.fill: parent
        visible: root.strokeProgress > 0
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            strokeColor: Theme.colOnPrimary
            strokeWidth: 2
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            fillColor: "transparent"
            startX: root.width * 0.25
            startY: root.height * 0.5

            PathLine {
                x: root.width * (0.25 + 0.17 * root.firstStroke)
                y: root.height * (0.5 + 0.17 * root.firstStroke)
            }
            PathLine {
                x: root.width * (0.25 + 0.17 * root.firstStroke + 0.33 * root.secondStroke)
                y: root.height * (0.5 + 0.17 * root.firstStroke - 0.34 * root.secondStroke)
            }
        }
    }

    Behavior on progress {
        NumberAnimation {
            duration: Theme.duration.spatialFast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.standard
        }
    }
}
