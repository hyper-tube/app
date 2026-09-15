import QtQuick
import HtMusic

Row {
    id: root

    property real markSize: 18
    property real labelSize: Theme.font.smallie
    property color colLabel: Theme.colOnSurfaceVariant

    spacing: Math.round(root.markSize * 0.55)

    Image {
        anchors.verticalCenter: parent.verticalCenter
        source: "qrc:/icons/ht-music.svg"
        sourceSize.width: root.markSize
        sourceSize.height: root.markSize
        width: root.markSize
        height: root.markSize
    }

    StyledText {
        anchors.verticalCenter: parent.verticalCenter
        text: AppInfo.name
        title: true
        font.pixelSize: root.labelSize
        color: root.colLabel
    }
}
