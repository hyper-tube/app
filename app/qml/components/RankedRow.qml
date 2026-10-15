import QtQuick
import HtMusic

RippleSurface {
    id: root

    required property var entry

    signal menuRequested(real x, real y)

    implicitHeight: Theme.sized(68)
    rounding: Theme.shape.card
    Accessible.name: root.entry.title + ", " + root.entry.subtitle + ", " + root.entry.rankLabel

    Artwork {
        id: cover

        anchors.left: parent.left
        anchors.leftMargin: Theme.spaced(10)
        anchors.verticalCenter: parent.verticalCenter
        width: Theme.sized(48)
        height: Theme.sized(48)
        artId: root.entry.artId
        rounding: root.entry.circular ? Theme.shape.avatar : Theme.shape.artwork
    }

    Column {
        id: ranking

        anchors.left: cover.right
        anchors.leftMargin: Theme.spaced(10)
        anchors.verticalCenter: parent.verticalCenter
        width: Theme.sized(30)

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
            iconSize: Theme.sized(20)
            fill: 1
            color: root.entry.movement === "up" ? Theme.colPrimary
                : root.entry.movement === "down" ? Theme.colError : Theme.colInactive
        }
    }

    Column {
        anchors.left: ranking.right
        anchors.leftMargin: Theme.spaced(10)
        anchors.right: parent.right
        anchors.rightMargin: Theme.spaced(14)
        anchors.verticalCenter: parent.verticalCenter
        spacing: Theme.spaced(4)

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
