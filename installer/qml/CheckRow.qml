import QtQuick
import HtMusic

RippleSurface {
    id: root

    property string title: ""
    property string caption: ""
    property bool checked: false

    signal toggled(bool value)

    implicitHeight: Math.max(52, lines.implicitHeight + 20)
    rounding: Theme.rounding.small
    onClicked: root.toggled(!root.checked)

    CheckMark {
        id: tick

        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        checked: root.checked
    }

    Column {
        id: lines

        anchors.left: tick.right
        anchors.leftMargin: 14
        anchors.right: parent.right
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2

        StyledText {
            width: parent.width
            text: root.title
            title: true
            wrapMode: Text.WordWrap
        }

        StyledText {
            width: parent.width
            visible: root.caption.length > 0
            text: root.caption
            wrapMode: Text.WordWrap
            font.pixelSize: Theme.font.smaller
            color: Theme.colOnSurfaceVariant
        }
    }
}
