import QtQuick
import HtMusic

RippleSurface {
    id: root

    required property Plugin plugin

    readonly property bool faulty: root.plugin.health === Plugin.Error

    property real faultReveal: root.faulty ? 1 : 0

    signal openRequested

    implicitHeight: 74
    rounding: Theme.rounding.normal
    Accessible.role: Accessible.Button
    Accessible.name: root.plugin.info.name

    Behavior on faultReveal {
        NumberAnimation {
            duration: Theme.duration.spatialFast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveFastSpatial
        }
    }

    Rectangle {
        id: tile

        x: 12
        anchors.verticalCenter: parent.verticalCenter
        width: 46
        height: 46
        radius: Theme.rounding.small
        color: root.plugin.enabled ? Theme.colPrimaryContainer : Theme.colLayer3

        Behavior on color {
            ColorAnimation {
                duration: Theme.duration.fast
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveEffects
            }
        }

        PluginGlyph {
            anchors.centerIn: parent
            info: root.plugin.info
            glyphSize: 23
            fill: root.plugin.enabled ? 1 : 0
            colGlyph: root.plugin.enabled ? Theme.colOnPrimaryContainer
                : Theme.colOnSurfaceVariant
        }

        Rectangle {
            x: parent.width - width + 3
            y: -3
            width: 13
            height: 13
            radius: width / 2
            color: Theme.colError
            border.width: 2
            border.color: Theme.colLayer1
            opacity: root.faultReveal
            scale: 0.3 + 0.7 * root.faultReveal
        }
    }

    Column {
        anchors.left: tile.right
        anchors.leftMargin: 14
        anchors.right: toggle.left
        anchors.rightMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        spacing: 3

        StyledText {
            width: parent.width
            text: root.plugin.info.name
            title: true
            elide: Text.ElideRight
            maximumLineCount: 1
        }

        StyledText {
            width: parent.width
            text: root.faulty ? root.plugin.status : root.plugin.info.description
            font.pixelSize: Theme.font.smaller
            color: root.faulty ? Theme.colError : Theme.colOnSurfaceVariant
            elide: Text.ElideRight
            maximumLineCount: 1
        }
    }

    ToggleSwitch {
        id: toggle

        anchors.right: parent.right
        anchors.rightMargin: 14
        anchors.verticalCenter: parent.verticalCenter
        checked: root.plugin.enabled
        Accessible.name: qsTr("Enable %1").arg(root.plugin.info.name)
        onToggled: value => root.plugin.enabled = value
    }

    onClicked: root.openRequested()
}
