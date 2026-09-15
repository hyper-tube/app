import QtQuick
import HtMusic

RippleSurface {
    id: root

    required property colorSourceInfo modelData

    implicitHeight: labels.implicitHeight + 16
    rounding: Theme.rounding.verysmall
    interactive: SystemTheme.sources.length > 1

    Sym {
        id: glyph

        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        text: root.modelData.active ? "check_circle" : "radio_button_unchecked"
        iconSize: Theme.font.small
        fill: root.modelData.active ? 1 : 0
        color: root.modelData.active ? Theme.colPrimary : Theme.colOutline
    }

    Column {
        id: labels

        anchors.left: glyph.right
        anchors.leftMargin: 10
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        spacing: 1

        StyledText {
            width: parent.width
            text: root.modelData.name
            title: root.modelData.active
            font.pixelSize: Theme.font.smallie
            elide: Text.ElideRight
        }

        StyledText {
            width: parent.width
            text: root.modelData.location
            font.pixelSize: Theme.font.smallest
            color: Theme.colOnSurfaceVariant
            elide: Text.ElideMiddle
        }
    }

    onClicked: SystemTheme.select(root.modelData.id)
}
