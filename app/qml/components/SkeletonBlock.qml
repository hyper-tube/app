import QtQuick
import HtMusic

Rectangle {
    id: root

    property real phase: 0
    property color colBase: Theme.colLayer3

    radius: Theme.rounding.verysmall

    gradient: Gradient {
        orientation: Gradient.Horizontal

        GradientStop {
            position: Math.max(0, Math.min(1, root.phase - 0.25))
            color: root.colBase
        }
        GradientStop {
            position: Math.max(0, Math.min(1, root.phase))
            color: ColorUtils.mix(Theme.colOnSurface, root.colBase, 0.1)
        }
        GradientStop {
            position: Math.max(0, Math.min(1, root.phase + 0.25))
            color: root.colBase
        }
    }
}
