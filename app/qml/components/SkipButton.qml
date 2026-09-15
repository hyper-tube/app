import QtQuick
import HtMusic

RippleSurface {
    id: root

    property bool forward: false
    property real morph: 0
    property real iconSize: 26
    property real diameter: 40

    readonly property real trackGlyph: Math.max(0, Math.min(1, 1 - root.morph))
    readonly property real stepGlyph: Math.max(0, Math.min(1, root.morph))

    implicitWidth: root.diameter
    implicitHeight: root.diameter
    rounding: Theme.rounding.full
    colState: Theme.colOnSurface
    acceptedButtons: Qt.LeftButton
    tooltip: Accessible.name
    Accessible.role: Accessible.Button

    Item {
        anchors.fill: parent
        scale: root.pressed ? 0.86 : 1

        Behavior on scale {
            NumberAnimation {
                duration: Theme.duration.spatialFast
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveFastSpatial
            }
        }

        Sym {
            anchors.centerIn: parent
            text: root.forward ? "skip_next" : "skip_previous"
            iconSize: root.iconSize
            fill: 1
            color: Theme.colOnSurfaceVariant
            opacity: root.trackGlyph
            scale: 0.6 + 0.4 * root.trackGlyph
            rotation: (root.forward ? 120 : -120) * root.stepGlyph
        }

        Sym {
            anchors.centerIn: parent
            text: root.forward ? "forward_30" : "replay_10"
            iconSize: root.iconSize
            color: Theme.colOnSurfaceVariant
            opacity: root.stepGlyph
            scale: 0.6 + 0.4 * root.stepGlyph
            rotation: (root.forward ? -120 : 120) * root.trackGlyph
        }
    }
}
