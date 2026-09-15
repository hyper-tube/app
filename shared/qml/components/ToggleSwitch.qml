import QtQuick
import HtMusic

RippleSurface {
    id: root

    property bool checked: false
    property real reveal: root.checked ? 1 : 0

    readonly property real thumbSize: 14 + 6 * root.reveal
    readonly property real thumbInset: (root.height - root.thumbSize) / 2
    readonly property real travel: root.width - root.thumbSize - root.thumbInset * 2

    signal toggled(bool value)

    implicitWidth: 44
    implicitHeight: 26
    rounding: Theme.rounding.full
    colBackground: ColorUtils.mix(Theme.colPrimary, Theme.colLayer4, root.reveal)
    colState: root.checked ? Theme.colOnPrimary : Theme.colOnSurface
    Accessible.role: Accessible.CheckBox
    Accessible.checked: root.checked

    Behavior on reveal {
        NumberAnimation {
            duration: Theme.duration.spatialFast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveFastSpatial
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: Theme.rounding.full
        color: "transparent"
        border.width: 2 * (1 - root.reveal)
        border.color: Theme.colOutline
    }

    Rectangle {
        x: root.thumbInset + root.travel * root.reveal
        anchors.verticalCenter: parent.verticalCenter
        width: root.thumbSize
        height: root.thumbSize
        radius: Theme.rounding.full
        color: ColorUtils.mix(Theme.colOnPrimary, Theme.colOutline, root.reveal)
    }

    onClicked: root.toggled(!root.checked)
}
