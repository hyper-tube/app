import QtQuick
import QtQuick.Shapes
import HtMusic

RippleSurface {
    id: root

    property bool playing: false
    property real iconSize: 28
    property real diameter: 50
    property real morph: root.playing ? 1 : 0

    tooltip: Accessible.name

    implicitWidth: root.diameter
    implicitHeight: root.diameter
    rounding: Theme.rounding.full
    colBackground: Theme.colPrimary
    colState: Theme.colOnPrimary
    Accessible.name: root.playing ? qsTr("Pause") : qsTr("Play")

    Shape {
        id: glyph

        readonly property real unit: root.iconSize / 24

        anchors.centerIn: parent
        width: root.iconSize
        height: root.iconSize
        preferredRendererType: Shape.CurveRenderer
        scale: root.pressed ? 0.86 : 1

        ShapePath {
            strokeWidth: -1
            fillColor: Theme.colOnPrimary
            startX: (8 - 2 * root.morph) * glyph.unit
            startY: (5 - root.morph) * glyph.unit

            PathLine {
                x: (13 - 3 * root.morph) * glyph.unit
                y: (8.18 - 4.18 * root.morph) * glyph.unit
            }
            PathLine {
                x: (13 - 3 * root.morph) * glyph.unit
                y: (8.18 + 11.82 * root.morph) * glyph.unit
            }
            PathLine {
                x: (13 + 1 * root.morph) * glyph.unit
                y: (8.18 + 11.82 * root.morph) * glyph.unit
            }
            PathLine {
                x: (13 + 1 * root.morph) * glyph.unit
                y: (8.18 - 4.18 * root.morph) * glyph.unit
            }
            PathLine {
                x: (19 - 1 * root.morph) * glyph.unit
                y: (12 - 8 * root.morph) * glyph.unit
            }
            PathLine {
                x: (19 - 1 * root.morph) * glyph.unit
                y: (12 + 8 * root.morph) * glyph.unit
            }
            PathLine {
                x: (8 - 2 * root.morph) * glyph.unit
                y: (19 + 1 * root.morph) * glyph.unit
            }
            PathLine {
                x: (8 - 2 * root.morph) * glyph.unit
                y: (5 - 1 * root.morph) * glyph.unit
            }
        }

        Behavior on scale {
            NumberAnimation {
                duration: Theme.duration.spatialFast
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveFastSpatial
            }
        }
    }

    Behavior on morph {
        NumberAnimation {
            duration: Theme.duration.spatialFast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.standard
        }
    }
}
