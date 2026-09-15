import QtQuick
import QtQuick.Controls
import HtMusic
import HtMusic.Setup

Item {
    id: root

    property bool accepted: false

    signal back
    signal agreed

    StyledText {
        id: heading

        anchors.left: parent.left
        anchors.leftMargin: Theme.size.gutter * 2
        anchors.top: parent.top
        anchors.topMargin: Theme.size.gutter
        text: qsTr("License agreement")
        title: true
        font.pixelSize: Theme.font.huge
    }

    Rectangle {
        id: plate

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: Theme.size.gutter * 2
        anchors.rightMargin: Theme.size.gutter * 2
        anchors.top: heading.bottom
        anchors.topMargin: 16
        anchors.bottom: footer.top
        anchors.bottomMargin: 16
        radius: Theme.rounding.normal
        color: Theme.colLayer1

        Flickable {
            id: scroller

            anchors.fill: parent
            anchors.margins: 18
            contentWidth: width
            contentHeight: body.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: PageScrollBar {}

            StyledText {
                id: body

                anchors.horizontalCenter: parent.horizontalCenter
                width: Math.min(body.implicitWidth, scroller.width - 12)
                text: Session.licenseText
                wrapMode: Text.WordWrap
                mono: true
                font.pixelSize: Theme.font.smallie
                color: Theme.colOnSurfaceVariant
            }
        }
    }

    Item {
        id: footer

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 88

        RippleSurface {
            id: consent

            anchors.left: parent.left
            anchors.leftMargin: Theme.size.gutter * 2 - 8
            anchors.verticalCenter: parent.verticalCenter
            width: agreement.implicitWidth + tick.width + 28
            height: 40
            rounding: Theme.rounding.small
            onClicked: root.accepted = !root.accepted

            CheckMark {
                id: tick

                anchors.left: parent.left
                anchors.leftMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                checked: root.accepted
            }

            StyledText {
                id: agreement

                anchors.left: tick.right
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("I accept the terms of the license")
            }
        }

        Row {
            anchors.right: parent.right
            anchors.rightMargin: Theme.size.gutter * 2
            anchors.verticalCenter: parent.verticalCenter
            spacing: 12

            PillButton {
                text: qsTr("Back")
                onClicked: root.back()
            }

            PillButton {
                text: qsTr("Continue")
                toggled: true
                interactive: root.accepted
                opacity: root.accepted ? 1 : 0.5
                onClicked: root.agreed()
            }
        }
    }
}
