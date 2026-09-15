import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import HtMusic
import HtMusic.Setup

Item {
    id: root

    readonly property bool ready: Session.directoryComplaint.length === 0

    signal back
    signal install

    StyledText {
        id: heading

        anchors.left: parent.left
        anchors.leftMargin: Theme.size.gutter * 2
        anchors.top: parent.top
        anchors.topMargin: Theme.size.gutter
        text: qsTr("Options")
        title: true
        font.pixelSize: Theme.font.huge
    }

    Flickable {
        id: scroller

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: Theme.size.gutter * 2
        anchors.rightMargin: Theme.size.gutter * 2
        anchors.top: heading.bottom
        anchors.topMargin: 12
        anchors.bottom: footer.top
        contentWidth: width
        contentHeight: body.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: PageScrollBar {}

        TapHandler {
            onPressedChanged: {
                if (pressed)
                    folder.clearFocus();
            }
        }

        Column {
            id: body

            width: scroller.width - 16
            spacing: 4

            StyledText {
                text: qsTr("Install for")
                title: true
                color: Theme.colOnSurfaceVariant
                font.pixelSize: Theme.font.smaller
                bottomPadding: 4
            }

            Row {
                width: parent.width
                spacing: 10

                ScopeChoice {
                    width: (parent.width - parent.spacing) / 2
                    interactive: Session.installedVersion.length === 0
                    title: qsTr("Just me")
                    caption: qsTr("Installs into your own folder. No administrator rights needed.")
                    icon: "person"
                    picked: Session.scope === Setup.Scope.CurrentUser
                    onChosen: Session.scope = Setup.Scope.CurrentUser
                }

                ScopeChoice {
                    width: (parent.width - parent.spacing) / 2
                    interactive: Session.installedVersion.length === 0
                    title: qsTr("All users")
                    caption: qsTr("Installs into Program Files. Windows will ask for permission.")
                    icon: "group"
                    picked: Session.scope === Setup.Scope.AllUsers
                    onChosen: Session.scope = Setup.Scope.AllUsers
                }
            }

            Item {
                width: 1
                height: 16
            }

            Row {
                width: parent.width
                spacing: 10

                InputField {
                    id: folder

                    width: parent.width - browse.width - 10
                    label: qsTr("Location")
                    enabled: Session.installedVersion.length === 0
                    text: Session.directory
                    onTextChanged: Session.directory = text
                }

                PillButton {
                    id: browse

                    anchors.verticalCenter: folder.verticalCenter
                    interactive: Session.installedVersion.length === 0
                    text: qsTr("Browse")
                    onClicked: picker.open()
                }
            }

            StyledText {
                width: parent.width
                topPadding: 6
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.font.smaller
                color: root.ready ? Theme.colOnSurfaceVariant : Theme.colError
                text: Session.complaint.length > 0 ? Session.complaint : root.ready
                    ? qsTr("%1 needed, %2 free").arg(Session.readable(Session.requiredBytes))
                        .arg(Session.readable(Session.availableBytes))
                    : Session.directoryComplaint
            }

            Item {
                width: 1
                height: 16
            }

            StyledText {
                text: qsTr("Additional settings")
                title: true
                color: Theme.colOnSurfaceVariant
                font.pixelSize: Theme.font.smaller
                bottomPadding: 4
            }

            Grid {
                width: parent.width
                columns: 2
                spacing: 10

                CheckRow {
                    width: (parent.width - parent.spacing) / 2
                    checked: Session.desktopShortcut
                    title: qsTr("Desktop shortcut")
                    onToggled: value => Session.desktopShortcut = value
                }

                CheckRow {
                    width: (parent.width - parent.spacing) / 2
                    checked: Session.startMenuShortcut
                    title: qsTr("Start menu shortcut")
                    onToggled: value => Session.startMenuShortcut = value
                }

                CheckRow {
                    width: (parent.width - parent.spacing) / 2
                    checked: Session.launchAtSignIn
                    title: qsTr("Start when I sign in")
                    caption: qsTr("Opens %1 after Windows starts.").arg(Session.productName)
                    onToggled: value => Session.launchAtSignIn = value
                }

                CheckRow {
                    width: (parent.width - parent.spacing) / 2
                    checked: Session.urlScheme
                    title: qsTr("Open %1 links").arg(Session.urlSchemeName)
                    caption: qsTr("Lets other applications hand links to %1.").arg(Session.productName)
                    onToggled: value => Session.urlScheme = value
                }
            }
        }
    }

    Item {
        id: footer

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 88

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
                icon: Session.elevationNeeded ? "shield_person" : ""
                text: qsTr("Install")
                toggled: true
                interactive: root.ready
                opacity: root.ready ? 1 : 0.5
                onClicked: root.install()
            }
        }
    }

    FolderDialog {
        id: picker

        title: qsTr("Choose a folder")
        onAccepted: Session.chooseDirectory(picker.selectedFolder)
    }
}
