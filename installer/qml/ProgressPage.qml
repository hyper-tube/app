import QtQuick
import HtMusic
import HtMusic.Setup

Item {
    id: root

    readonly property real reach: Session.job.progress

    Column {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -24
        width: Math.min(parent.width - Theme.size.gutter * 6, 520)
        spacing: 22

        Wordmark {
            anchors.horizontalCenter: parent.horizontalCenter
            markSize: 56
            labelSize: Theme.font.huge
            colLabel: Theme.colOnSurface
        }

        StyledText {
            anchors.horizontalCenter: parent.horizontalCenter
            text: {
                switch (Session.mode) {
                case Setup.Mode.Update:
                    return qsTr("Updating %1").arg(Session.productName);
                case Setup.Mode.Repair:
                    return qsTr("Repairing %1").arg(Session.productName);
                case Setup.Mode.Uninstall:
                    return qsTr("Removing %1").arg(Session.productName);
                default:
                    return qsTr("Installing %1").arg(Session.productName);
                }
            }
            color: Theme.colOnSurfaceVariant
            font.pixelSize: Theme.font.large
        }

        ProgressBar {
            width: parent.width
            value: root.reach
            indeterminate: root.reach <= 0.001
        }

        Item {
            width: parent.width
            height: 20

            StyledText {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - 60
                text: Session.job.step
                elide: Text.ElideMiddle
                font.pixelSize: Theme.font.smaller
                color: Theme.colOnSurfaceVariant
            }

            StyledText {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                text: Math.round(root.reach * 100) + "%"
                font.pixelSize: Theme.font.smaller
                color: Theme.colOnSurfaceVariant
            }
        }
    }

    PillButton {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 92
        text: qsTr("Cancel")
        ghost: true
        visible: Session.job.running && Session.mode !== Setup.Mode.Uninstall
        onClicked: Session.job.cancel()
    }
}
