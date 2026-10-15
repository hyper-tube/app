import QtQuick
import HtMusic

Item {
    id: root

    required property var note
    property bool scoped: false
    property real scopeWidth: 0

    readonly property real gutter: root.scoped ? Math.min(Theme.sized(168), root.scopeWidth + Theme.spaced(34)) : Theme.spaced(20)
    readonly property real firstLine: metrics.height
    readonly property bool hasCommit: root.note.commit.length > 0

    implicitHeight: Math.max(message.implicitHeight, lead.implicitHeight) + Theme.spaced(12)

    FontMetrics {
        id: metrics

        font: message.font
    }

    Column {
        id: lead

        width: root.gutter - Theme.spaced(12)
        y: Math.max(0, (root.firstLine - Theme.sized(22)) / 2)
        spacing: Theme.spaced(4)

        Rectangle {
            visible: root.note.scope.length > 0
            width: Math.min(lead.width, scopeLabel.implicitWidth + Theme.spaced(18))
            height: Theme.sized(22)
            radius: Theme.shape.badge
            color: Theme.colSecondaryContainer

            StyledText {
                id: scopeLabel

                anchors.centerIn: parent
                width: Math.min(implicitWidth, parent.width - Theme.spaced(18))
                text: root.note.scope
                title: true
                font.pixelSize: Theme.font.smaller
                color: Theme.colOnSecondaryContainer
                elide: Text.ElideRight
            }
        }

        Rectangle {
            visible: root.note.breaking
            width: Math.min(lead.width, breakingLabel.implicitWidth + Theme.spaced(18))
            height: Theme.sized(22)
            radius: Theme.shape.badge
            color: "transparent"
            border.width: 1
            border.color: Theme.colError

            StyledText {
                id: breakingLabel

                anchors.centerIn: parent
                width: Math.min(implicitWidth, parent.width - Theme.spaced(18))
                text: qsTr("Breaking")
                title: true
                font.pixelSize: Theme.font.smaller
                color: Theme.colError
                elide: Text.ElideRight
            }
        }

        Item {
            visible: root.note.scope.length === 0 && !root.note.breaking
            width: lead.width
            height: Theme.sized(22)

            Rectangle {
                x: root.scoped ? Theme.spaced(8) : Theme.spaced(2)
                anchors.verticalCenter: parent.verticalCenter
                width: Theme.sized(6)
                height: Theme.sized(6)
                radius: width / 2
                color: Theme.colPrimary
            }
        }
    }

    StyledText {
        id: message

        x: root.gutter
        width: root.width - root.gutter - (root.hasCommit ? commit.width + Theme.spaced(12) : 0)
        text: root.note.text
        textFormat: Text.MarkdownText
        wrapMode: Text.Wrap
        font.pixelSize: Theme.font.small
        lineHeight: 1.12
        color: Theme.colOnSurface
        linkColor: Theme.colPrimary
        onLinkActivated: link => Qt.openUrlExternally(link)

        HoverHandler {
            cursorShape: message.hoveredLink.length > 0 ? Qt.PointingHandCursor : Qt.ArrowCursor
        }
    }

    RippleSurface {
        id: commit

        anchors.right: parent.right
        y: Math.max(0, (root.firstLine - height) / 2)
        visible: root.hasCommit
        width: hash.implicitWidth + Theme.spaced(16)
        height: Theme.sized(24)
        rounding: Theme.shape.chip
        tooltip: qsTr("Open this change")
        Accessible.name: qsTr("Open change %1").arg(root.note.commit)
        onClicked: Qt.openUrlExternally(root.note.commitUrl)

        StyledText {
            id: hash

            anchors.centerIn: parent
            text: root.note.commit
            mono: true
            font.pixelSize: Theme.font.smaller
            color: commit.hovered ? Theme.colPrimary : Theme.colInactive
        }
    }
}
