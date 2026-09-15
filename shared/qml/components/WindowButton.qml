import QtQuick
import HtMusic

RippleSurface {
    id: root

    property string icon: ""
    property real iconSize: Theme.font.normal
    property bool systemHovered: false
    property bool systemPressed: false

    tooltip: Accessible.name

    implicitWidth: 32
    implicitHeight: 32
    rounding: Theme.rounding.full
    colBackground: Theme.colLayer2
    colState: Theme.colOnSurface
    stateOpacity: root.pressed || root.systemPressed ? Theme.state.press
        : root.hovered || root.systemHovered ? Theme.state.hover
        : 0

    Sym {
        anchors.centerIn: parent
        text: root.icon
        iconSize: root.iconSize
        weight: Font.Bold
        grade: 200
        color: Theme.colOnSurface
        scale: root.pressed || root.systemPressed ? 0.86 : 1

        Behavior on scale {
            NumberAnimation {
                duration: Theme.duration.spatialFast
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveFastSpatial
            }
        }
    }
}
