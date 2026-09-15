import QtQuick
import HtMusic

Item {
    id: root

    property string title: ""
    property string subtitle: ""
    property bool moreAvailable: false
    property bool moreGhost: false

    signal moreRequested

    implicitHeight: Math.max(column.implicitHeight, more.visible ? more.implicitHeight : 0)

    Column {
        id: column

        anchors.left: parent.left
        anchors.right: more.visible ? more.left : parent.right
        anchors.rightMargin: more.visible ? 16 : 0
        anchors.verticalCenter: parent.verticalCenter
        spacing: 1

        StyledText {
            width: parent.width
            elide: Text.ElideRight
            text: root.title
            title: true
            font.pixelSize: Theme.font.huge
            color: Theme.colOnSurface
        }

        StyledText {
            width: parent.width
            elide: Text.ElideRight
            visible: root.subtitle.length > 0
            text: root.subtitle
            font.pixelSize: Theme.font.smallie
            color: Theme.colInactive
        }
    }

    PillButton {
        id: more

        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        visible: root.moreAvailable
        ghost: root.moreGhost
        text: qsTr("See all")
        onClicked: root.moreRequested()
    }
}
