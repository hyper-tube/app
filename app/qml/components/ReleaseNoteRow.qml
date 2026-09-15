import QtQuick
import HtMusic

Item {
    id: root

    required property var note
    property bool scoped: false
    property real scopeWidth: 0

    readonly property real gutter: root.scoped ? Math.min(168, root.scopeWidth + 34) : 20
    readonly property real firstLine: metrics.height
    readonly property bool hasCommit: root.note.commit.length > 0

    implicitHeight: Math.max(message.implicitHeight, lead.implicitHeight) + 12

    FontMetrics {
        id: metrics

        font: message.font
    }

    Column {
        id: lead

        width: root.gutter - 12
        y: Math.max(0, (root.firstLine - 22) / 2)
        spacing: 4

        Rectangle {
            visible: root.note.scope.length > 0
            width: Math.min(lead.width, scopeLabel.implicitWidth + 18)
            height: 22
            radius: height / 2
            color: Theme.colSecondaryContainer

            StyledText {
                id: scopeLabel

                anchors.centerIn: parent
                width: Math.min(implicitWidth, parent.width - 18)
                text: root.note.scope
                title: true
                font.pixelSize: Theme.font.smaller
                color: Theme.colOnSecondaryContainer
                elide: Text.ElideRight
            }
        }

        Rectangle {
            visible: root.note.breaking
            width: Math.min(lead.width, breakingLabel.implicitWidth + 18)
            height: 22
            radius: height / 2
            color: "transparent"
            border.width: 1
            border.color: Theme.colError

            StyledText {
                id: breakingLabel

                anchors.centerIn: parent
                width: Math.min(implicitWidth, parent.width - 18)
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
            height: 22

            Rectangle {
                x: root.scoped ? 8 : 2
                anchors.verticalCenter: parent.verticalCenter
                width: 6
                height: 6
                radius: 3
                color: Theme.colPrimary
            }
        }
    }

    StyledText {
        id: message

        x: root.gutter
        width: root.width - root.gutter - (root.hasCommit ? commit.width + 12 : 0)
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
        width: hash.implicitWidth + 16
        height: 24
        rounding: Theme.rounding.full
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
