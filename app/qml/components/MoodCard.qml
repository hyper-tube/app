import QtQuick
import HtMusic

RippleSurface {
    id: root

    property string title: ""

    implicitWidth: Theme.sized(224)
    implicitHeight: Theme.sized(52)
    rounding: Theme.rounding.small
    colBackground: Theme.colLayer2

    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: Theme.spaced(8)
        width: Theme.sized(4)
        radius: Theme.shape.track
        color: Theme.colPrimary
    }

    StyledText {
        anchors.left: parent.left
        anchors.leftMargin: Theme.spaced(22)
        anchors.right: parent.right
        anchors.rightMargin: Theme.spaced(14)
        anchors.verticalCenter: parent.verticalCenter
        text: root.title
        title: true
        font.pixelSize: Theme.font.smallie
        elide: Text.ElideRight
    }
}
