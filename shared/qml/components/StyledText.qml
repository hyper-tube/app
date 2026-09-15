import QtQuick
import HtMusic

Text {
    id: root

    property bool title: false
    property bool mono: false

    renderType: Text.NativeRendering
    verticalAlignment: Text.AlignVCenter
    color: Theme.colOnSurface

    font {
        family: root.mono ? Theme.font.mono : Theme.font.main
        pixelSize: Theme.font.small
        hintingPreference: Font.PreferDefaultHinting
        variableAxes: root.title ? Theme.font.axesTitle : Theme.font.axes
    }

    Behavior on color {
        ColorAnimation {
            duration: Theme.duration.fast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveEffects
        }
    }
}
