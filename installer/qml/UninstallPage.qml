import QtQuick
import HtMusic
import HtMusic.Setup

Item {
    id: root

    readonly property real margin: Theme.size.gutter * 2

    signal back
    signal remove

    Column {
        anchors.left: parent.left
        anchors.leftMargin: root.margin
        anchors.right: parent.right
        anchors.rightMargin: root.margin
        anchors.top: parent.top
        anchors.topMargin: 44
        spacing: 24

        BrandHeader {
            caption: Session.installedVersion.length > 0
                ? qsTr("Version %1 is installed").arg(Session.installedVersion)
                : qsTr("Version %1").arg(Session.version)
        }

        StyledText {
            width: Math.min(parent.width, 560)
            wrapMode: Text.WordWrap
            font.pixelSize: Theme.font.large
            color: Theme.colOnSurfaceVariant
            text: qsTr("This will remove %1 from this computer.").arg(Session.productName)
        }

        CheckRow {
            width: Math.min(parent.width, 560)
            checked: Session.removeUserData
            title: qsTr("Also delete my settings, downloads and history")
            caption: qsTr("Leave this off to keep your music and preferences for next time.")
            onToggled: value => Session.removeUserData = value
        }
    }

    Item {
        anchors.left: parent.left
        anchors.leftMargin: root.margin
        anchors.right: parent.right
        anchors.rightMargin: root.margin
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 40
        height: language.height

        SelectField {
            id: language

            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: 178
            icon: "language"
            options: Localization.options
            value: Localization.language
            onPicked: choice => Localization.language = choice
        }

        Row {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            spacing: 12

            PillButton {
                text: qsTr("Cancel")
                onClicked: root.back()
            }

            PillButton {
                text: qsTr("Uninstall")
                toggled: true
                onClicked: root.remove()
            }
        }
    }
}
