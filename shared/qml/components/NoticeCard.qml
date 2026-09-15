import QtQuick
import HtMusic

Item {
    id: root

    property string icon: "info"
    property color colAccent: Theme.colPrimary
    property string title: ""
    property string caption: ""

    default property alias content: extra.data

    implicitHeight: surface.height

    Rectangle {
        id: surface

        width: parent.width
        height: layout.implicitHeight + 32
        radius: Theme.rounding.small
        color: Theme.colLayer2

        Column {
            id: layout

            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.top: parent.top
            anchors.topMargin: 16
            spacing: 12

            Item {
                width: parent.width
                height: Math.max(glyph.implicitHeight, heading.implicitHeight)

                Sym {
                    id: glyph

                    anchors.left: parent.left
                    anchors.top: parent.top
                    text: root.icon
                    iconSize: Theme.font.larger
                    fill: 1
                    color: root.colAccent
                }

                Column {
                    id: heading

                    anchors.left: glyph.right
                    anchors.leftMargin: 14
                    anchors.right: parent.right
                    anchors.top: parent.top
                    spacing: 2

                    StyledText {
                        width: parent.width
                        visible: root.title.length > 0
                        text: root.title
                        title: true
                        wrapMode: Text.WordWrap
                    }

                    StyledText {
                        width: parent.width
                        visible: root.caption.length > 0
                        text: root.caption
                        font.pixelSize: Theme.font.smaller
                        color: Theme.colOnSurfaceVariant
                        wrapMode: Text.WordWrap
                    }
                }
            }

            Column {
                id: extra

                width: parent.width
                visible: extra.height > 0
                spacing: 2
            }
        }
    }
}
