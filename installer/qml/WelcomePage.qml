import QtQuick
import HtMusic
import HtMusic.Setup

Item {
    id: root

    readonly property bool maintaining: Session.mode !== Setup.Mode.Install
    readonly property real margin: Theme.size.gutter * 2

    signal install
    signal customize
    signal showLicense
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
            caption: root.maintaining && Session.installedVersion.length > 0
                ? qsTr("Version %1 is installed").arg(Session.installedVersion)
                : qsTr("Version %1").arg(Session.version)
        }

        StyledText {
            width: Math.min(parent.width, 560)
            wrapMode: Text.WordWrap
            font.pixelSize: Theme.font.large
            color: Theme.colOnSurfaceVariant
            text: {
                switch (Session.mode) {
                case Setup.Mode.Update:
                    return (Session.downgrade
                        ? qsTr("This will downgrade %1 to version %2 and keep your settings, downloads and history.")
                        : qsTr("This will update %1 to version %2 and keep your settings, downloads and history."))
                        .arg(Session.productName).arg(Session.version);
                case Setup.Mode.Repair:
                    return qsTr("This will reinstall the files %1 needs and leave everything else alone.")
                        .arg(Session.productName);
                default:
                    return qsTr("A native YouTube Music player. Installing takes about a minute and needs %1 of disk space.")
                        .arg(Session.readable(Session.requiredBytes));
                }
            }
        }

        StyledText {
            width: Math.min(parent.width, 560)
            visible: Session.complaint.length > 0
            text: Session.complaint
            color: Theme.colError
            wrapMode: Text.WordWrap
        }
    }

    Item {
        anchors.left: parent.left
        anchors.leftMargin: root.margin
        anchors.right: parent.right
        anchors.rightMargin: root.margin
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 40
        height: language.height + 8 + note.implicitHeight

        SelectField {
            id: language

            anchors.left: parent.left
            anchors.top: parent.top
            width: 178
            icon: "language"
            options: Localization.options
            value: Localization.language
            onPicked: choice => Localization.language = choice
        }

        Row {
            id: actions

            anchors.right: parent.right
            anchors.verticalCenter: language.verticalCenter
            spacing: 12

            PillButton {
                text: qsTr("Uninstall")
                visible: root.maintaining
                onClicked: root.remove()
            }

            PillButton {
                text: qsTr("Customize")
                onClicked: root.customize()
            }

            PillButton {
                text: {
                    switch (Session.mode) {
                    case Setup.Mode.Update:
                        return Session.downgrade ? qsTr("Downgrade") : qsTr("Update");
                    case Setup.Mode.Repair:
                        return qsTr("Repair");
                    default:
                        return qsTr("Install");
                    }
                }
                toggled: true
                onClicked: root.install()
            }
        }

        StyledText {
            id: note

            anchors.right: parent.right
            anchors.top: language.bottom
            anchors.topMargin: 8
            text: qsTr("By installing you agree to the <a href='license'>License Agreement</a>.")
            textFormat: Text.RichText
            font.pixelSize: Theme.font.smaller
            color: Theme.colOnSurfaceVariant
            linkColor: Theme.colPrimary
            onLinkActivated: root.showLicense()
        }
    }
}
