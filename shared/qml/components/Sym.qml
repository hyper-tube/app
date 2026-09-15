import QtQuick
import HtMusic

Text {
    id: root

    property real iconSize: Theme.font.normal
    property real fill: 0
    property int weight: Font.Normal + (Font.DemiBold - Font.Normal) * root.steppedFill
    property int grade: 0

    readonly property real steppedFill: fill.toFixed(1)

    renderType: Text.NativeRendering
    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignHCenter
    color: Theme.colOnSurfaceVariant

    font {
        family: Theme.font.icon
        pixelSize: root.iconSize
        hintingPreference: Font.PreferNoHinting
        weight: root.weight
        variableAxes: ({ "FILL": root.steppedFill, "GRAD": root.grade, "opsz": root.iconSize })
    }

    Behavior on fill {
        NumberAnimation {
            duration: Theme.duration.fast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveEffects
        }
    }

    Behavior on color {
        ColorAnimation {
            duration: Theme.duration.fast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveEffects
        }
    }
}
