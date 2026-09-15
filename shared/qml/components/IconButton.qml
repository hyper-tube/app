import QtQuick
import HtMusic

RippleSurface {
    id: root

    property string icon: ""
    property real iconSize: Theme.font.larger
    property real iconFill: root.toggled || root.filled ? 1 : 0
    property real diameter: 40
    property bool toggled: false
    property bool filled: false
    property color colIcon: root.filled ? Theme.colOnPrimary
        : root.toggled ? Theme.colPrimary
        : Theme.colOnSurfaceVariant

    tooltip: Accessible.name
    acceptedButtons: Qt.LeftButton

    implicitWidth: diameter
    implicitHeight: diameter
    rounding: Theme.rounding.full
    colBackground: root.filled ? Theme.colPrimary : "transparent"
    colState: root.filled ? Theme.colOnPrimary : Theme.colOnSurface

    Sym {
        anchors.centerIn: parent
        text: root.icon
        iconSize: root.iconSize
        fill: root.iconFill
        color: root.colIcon
        scale: root.pressed ? 0.86 : 1

        Behavior on scale {
            NumberAnimation {
                duration: Theme.duration.spatialFast
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveFastSpatial
            }
        }
    }
}
