import QtQuick
import HtMusic

RippleSurface {
    id: root

    property string text: ""
    property string icon: ""
    property bool toggled: false
    property bool ghost: false
    property bool busy: false
    property int iconSize: Theme.font.normal
    property real horizontalPadding: 18
    property real leadingPadding: root.horizontalPadding
    property real busyReveal: root.busy ? 1 : 0

    readonly property color colLabel: root.toggled ? Theme.colOnSecondaryContainer
        : Theme.colOnSurfaceVariant

    implicitWidth: row.implicitWidth + root.leadingPadding + root.horizontalPadding
    implicitHeight: 38
    rounding: Theme.rounding.full
    colBackground: root.toggled ? Theme.colSecondaryContainer
        : root.ghost ? "transparent" : Theme.colLayer3
    colState: root.toggled ? Theme.colOnSecondaryContainer : Theme.colOnSurface

    Behavior on busyReveal {
        NumberAnimation {
            duration: Theme.duration.fast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveEffects
        }
    }

    Row {
        id: row

        x: root.leadingPadding
        anchors.verticalCenter: parent.verticalCenter
        spacing: root.icon ? 8 : 0

        Sym {
            id: glyph

            anchors.verticalCenter: parent.verticalCenter
            visible: root.icon.length > 0
            text: root.icon
            iconSize: root.iconSize
            fill: root.toggled ? 1 : 0
            color: root.colLabel
            opacity: 1 - root.busyReveal
            scale: 1 - 0.4 * root.busyReveal
        }

        StyledText {
            anchors.verticalCenter: parent.verticalCenter
            text: root.text
            title: root.toggled
            font.pixelSize: Theme.font.smallie
            color: root.colLabel
        }
    }

    BusySpinner {
        x: row.x + glyph.x + (glyph.width - width) / 2
        y: row.y + glyph.y + (glyph.height - height) / 2
        width: root.iconSize + 2
        height: width
        visible: glyph.visible && root.busyReveal > 0
        opacity: root.busyReveal
        colArc: root.colLabel
    }
}
