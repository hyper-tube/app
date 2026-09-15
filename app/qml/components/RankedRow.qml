import QtQuick
import HtMusic

RippleSurface {
    id: root

    required property var entry

    signal menuRequested(real x, real y)

    implicitHeight: 68
    rounding: Theme.rounding.normal
    Accessible.name: root.entry.title + ", " + root.entry.subtitle + ", " + root.entry.rankLabel

    Artwork {
        id: cover

        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        width: 48
        height: 48
        artId: root.entry.artId
        rounding: root.entry.circular ? Theme.rounding.full : Theme.rounding.small
    }

    Column {
        id: ranking

        anchors.left: cover.right
        anchors.leftMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        width: 30

        StyledText {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.entry.rank
            title: true
            font.pixelSize: Theme.font.small
            color: Theme.colOnSurface
        }

        Sym {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.entry.movement.length > 0
            text: root.entry.movement === "up" ? "arrow_drop_up"
                : root.entry.movement === "down" ? "arrow_drop_down" : "remove"
            iconSize: 20
            fill: 1
            color: root.entry.movement === "up" ? Theme.colPrimary
                : root.entry.movement === "down" ? Theme.colError : Theme.colInactive
        }
    }

    Column {
        anchors.left: ranking.right
        anchors.leftMargin: 10
        anchors.right: parent.right
        anchors.rightMargin: 14
        anchors.verticalCenter: parent.verticalCenter
        spacing: 4

        StyledText {
            width: parent.width
            text: root.entry.title
            title: true
            font.pixelSize: Theme.font.small
            elide: Text.ElideRight
        }

        StyledText {
            width: parent.width
            text: root.entry.subtitle
            font.pixelSize: Theme.font.smaller
            color: Theme.colOnSurfaceVariant
            elide: Text.ElideRight
        }
    }

    onRightClicked: (x, y) => root.menuRequested(x, y)
}
