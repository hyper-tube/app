import QtQuick
import HtMusic

RippleSurface {
    id: root

    property string title: ""

    implicitWidth: 224
    implicitHeight: 52
    rounding: Theme.rounding.small
    colBackground: Theme.colLayer2

    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: 8
        width: 4
        radius: Theme.rounding.full
        color: Theme.colPrimary
    }

    StyledText {
        anchors.left: parent.left
        anchors.leftMargin: 22
        anchors.right: parent.right
        anchors.rightMargin: 14
        anchors.verticalCenter: parent.verticalCenter
        text: root.title
        title: true
        font.pixelSize: Theme.font.smallie
        elide: Text.ElideRight
    }
}
