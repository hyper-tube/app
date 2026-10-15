import QtQuick
import HtMusic

Item {
    id: root

    property string title: ""
    property string caption: ""
    property real controlWidth: control.implicitWidth

    default property alias control: control.data

    implicitHeight: Math.max(Theme.sized(52), labels.implicitHeight + Theme.spaced(18), control.height + Theme.spaced(8))

    Column {
        id: labels

        anchors.left: parent.left
        anchors.right: control.left
        anchors.rightMargin: Theme.spaced(20)
        anchors.verticalCenter: parent.verticalCenter
        spacing: Theme.spaced(2)

        StyledText {
            width: parent.width
            text: root.title
            title: true
            elide: Text.ElideRight
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

    Item {
        id: control

        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        width: root.controlWidth
        height: childrenRect.height
    }
}
