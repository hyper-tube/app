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

    implicitHeight: Theme.sized(58) + Theme.sized(30) * root.trackReveal + Theme.sized(48) * root.retryReveal
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

        x: Theme.spaced(14)
        y: Theme.sized(18)
        info: DiscordPresence.info
        glyphSize: Theme.sized(21)
        colGlyph: DiscordPresence.connected ? Theme.colPrimary : Theme.colOnSurfaceVariant
    }

    Column {
        anchors.left: glyph.right
        anchors.leftMargin: Theme.spaced(12)
        anchors.right: quick.left
        anchors.rightMargin: Theme.spaced(12)
        y: Theme.sized(11)
        spacing: Theme.spaced(2)

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
        anchors.rightMargin: Theme.spaced(12)
        y: Theme.sized(16)
        checked: DiscordPresence.sharing
        Accessible.name: qsTr("Share what you are listening to")
        onToggled: value => DiscordPresence.sharing = value
    }

    Item {
        y: Theme.sized(58)
        width: parent.width
        height: Theme.sized(30) * root.trackReveal
        clip: true
        opacity: root.trackReveal

        Row {
            x: Theme.spaced(14)
            width: parent.width - Theme.spaced(28)
            anchors.verticalCenter: parent.verticalCenter
            spacing: Theme.spaced(8)

            Sym {
                anchors.verticalCenter: parent.verticalCenter
                text: "music_note"
                iconSize: Theme.font.small
                color: Theme.colPrimary
            }

            StyledText {
                width: parent.width - Theme.sized(26)
                text: DiscordPresence.shown.title
                font.pixelSize: Theme.font.smallie
                elide: Text.ElideRight
            }
        }
    }

    Item {
        y: Theme.sized(58) + Theme.sized(30) * root.trackReveal
        width: parent.width
        height: Theme.sized(48) * root.retryReveal
        clip: true
        opacity: root.retryReveal

        PillButton {
            x: Theme.spaced(14)
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Reconnect")
            icon: "refresh"
            toggled: true
            horizontalPadding: Theme.spaced(14)
            onClicked: DiscordPresence.reconnect()
        }
    }

    onClicked: PluginRegistry.openPage(DiscordPresence.info.id)
}
