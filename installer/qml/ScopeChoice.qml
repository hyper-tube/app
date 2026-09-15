import QtQuick
import HtMusic

RippleSurface {
    id: root

    property string title: ""
    property string caption: ""
    property string icon: ""
    property bool picked: false

    signal chosen

    implicitHeight: Math.max(66, lines.implicitHeight + 28)
    rounding: Theme.rounding.normal
    colBackground: root.picked ? Theme.colSecondaryContainer : Theme.colLayer1
    colState: root.picked ? Theme.colOnSecondaryContainer : Theme.colOnSurface
    onClicked: root.chosen()

    Sym {
        id: glyph

        anchors.left: parent.left
        anchors.leftMargin: 18
        anchors.verticalCenter: parent.verticalCenter
        text: root.icon
        iconSize: 22
        fill: root.picked ? 1 : 0
        color: root.picked ? Theme.colOnSecondaryContainer : Theme.colOnSurfaceVariant
    }

    Column {
        id: lines

        anchors.left: glyph.right
        anchors.leftMargin: 16
        anchors.right: tick.left
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2

        StyledText {
            width: parent.width
            text: root.title
            title: true
            elide: Text.ElideRight
            color: root.picked ? Theme.colOnSecondaryContainer : Theme.colOnSurface
        }

        StyledText {
            width: parent.width
            text: root.caption
            wrapMode: Text.WordWrap
            font.pixelSize: Theme.font.smaller
            color: root.picked ? Theme.colOnSecondaryContainer : Theme.colOnSurfaceVariant
        }
    }

    RadioMark {
        id: tick

        anchors.right: parent.right
        anchors.rightMargin: 18
        anchors.verticalCenter: parent.verticalCenter
        checked: root.picked
    }
}
