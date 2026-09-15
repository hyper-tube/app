import QtQuick
import HtMusic
import HtMusic.Setup

Row {
    id: root

    property string caption: ""
    property real markSize: 58

    spacing: 16

    Image {
        anchors.verticalCenter: parent.verticalCenter
        source: "qrc:/icons/ht-music.svg"
        sourceSize.width: root.markSize
        sourceSize.height: root.markSize
        width: root.markSize
        height: root.markSize
    }

    Column {
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2

        StyledText {
            text: Session.productName
            title: true
            font.pixelSize: Theme.font.display
            color: Theme.colOnSurface
        }

        StyledText {
            text: root.caption
            font.pixelSize: Theme.font.small
            color: Theme.colOnSurfaceVariant
        }
    }
}
