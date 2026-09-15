import QtQuick
import HtMusic

RippleSurface {
    id: root

    readonly property bool retryable: DiscordPresence.sharing && !DiscordPresence.connected
    readonly property bool showing: DiscordPresence.shown.valid
    readonly property color colStatus: DiscordPresence.health === Plugin.Error ? Theme.colError
        : DiscordPresence.health === Plugin.Warning ? Theme.colTertiary
        : Theme.colOnSurfaceVariant

    property real retryReveal: root.retryable ? 1 : 0
    property real trackReveal: root.showing ? 1 : 0

    implicitHeight: 58 + 30 * root.trackReveal + 48 * root.retryReveal
    rounding: Theme.rounding.small
    colBackground: Theme.colLayer2
    Accessible.role: Accessible.Button
    Accessible.name: qsTr("Discord presence")

    Behavior on retryReveal {
        NumberAnimation {
            duration: Theme.duration.spatialFast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveFastSpatial
        }
    }

    Behavior on trackReveal {
        NumberAnimation {
            duration: Theme.duration.spatialFast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveFastSpatial
        }
    }

    PluginGlyph {
        id: glyph

        x: 14
        y: 18
        info: DiscordPresence.info
        glyphSize: 21
        colGlyph: DiscordPresence.connected ? Theme.colPrimary : Theme.colOnSurfaceVariant
    }

    Column {
        anchors.left: glyph.right
        anchors.leftMargin: 12
        anchors.right: quick.left
        anchors.rightMargin: 12
        y: 11
        spacing: 2

        StyledText {
            width: parent.width
            text: qsTr("Discord presence")
            title: true
            elide: Text.ElideRight
        }

        StyledText {
            width: parent.width
            text: DiscordPresence.status
            font.pixelSize: Theme.font.smaller
            color: root.colStatus
            elide: Text.ElideRight
        }
    }

    ToggleSwitch {
        id: quick

        anchors.right: parent.right
        anchors.rightMargin: 12
        y: 16
        checked: DiscordPresence.sharing
        Accessible.name: qsTr("Share what you are listening to")
        onToggled: value => DiscordPresence.sharing = value
    }

    Item {
        y: 58
        width: parent.width
        height: 30 * root.trackReveal
        clip: true
        opacity: root.trackReveal

        Row {
            x: 14
            width: parent.width - 28
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8

            Sym {
                anchors.verticalCenter: parent.verticalCenter
                text: "music_note"
                iconSize: Theme.font.small
                color: Theme.colPrimary
            }

            StyledText {
                width: parent.width - 26
                text: DiscordPresence.shown.title
                font.pixelSize: Theme.font.smallie
                elide: Text.ElideRight
            }
        }
    }

    Item {
        y: 58 + 30 * root.trackReveal
        width: parent.width
        height: 48 * root.retryReveal
        clip: true
        opacity: root.retryReveal

        PillButton {
            x: 14
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Reconnect")
            icon: "refresh"
            toggled: true
            horizontalPadding: 14
            onClicked: DiscordPresence.reconnect()
        }
    }

    onClicked: PluginRegistry.openPage(DiscordPresence.info.id)
}
