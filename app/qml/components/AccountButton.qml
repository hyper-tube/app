import QtQuick
import HtMusic

Item {
    id: root

    property bool glass: false

    readonly property string label: Account.name.length > 0 ? Account.name : qsTr("Account")

    signal historyRequested

    implicitWidth: surface.implicitWidth
    implicitHeight: 40

    RippleSurface {
        id: surface

        tooltip: Account.signedIn ? qsTr("Account options") : qsTr("Sign in")

        implicitWidth: row.implicitWidth + 28
        anchors.fill: parent
        rounding: Theme.rounding.full
        colBackground: root.glass ? ColorUtils.withAlpha(Theme.colLayer2, 0.5)
            : Account.signedIn ? Theme.colLayer2 : Theme.colSecondaryContainer
        colState: Account.signedIn ? Theme.colOnSurface : Theme.colOnSecondaryContainer

        Row {
            id: row

            anchors.centerIn: parent
            spacing: 8

            Artwork {
                anchors.verticalCenter: parent.verticalCenter
                width: 26
                height: 26
                visible: Account.signedIn && Account.artId.length > 0
                artId: Account.artId
                rounding: Theme.rounding.full
            }

            Sym {
                anchors.verticalCenter: parent.verticalCenter
                visible: !Account.signedIn || Account.artId.length === 0
                text: Account.status === Account.Expired ? "person_alert" : "account_circle"
                iconSize: Theme.font.larger
                color: Account.signedIn ? Theme.colOnSurfaceVariant : Theme.colOnSecondaryContainer
            }

            StyledText {
                anchors.verticalCenter: parent.verticalCenter
                text: Account.busy ? qsTr("Signing in...")
                    : Account.signedIn ? root.label
                    : Account.status === Account.Expired ? qsTr("Sign in again")
                    : qsTr("Sign in")
                title: !Account.signedIn
                elide: Text.ElideRight
                maximumLineCount: 1
                font.pixelSize: Theme.font.smallie
                color: Account.signedIn ? Theme.colOnSurface : Theme.colOnSecondaryContainer
            }
        }

        onClicked: {
            if (Account.signedIn)
                menu.toggle();
            else
                Account.requestSignIn();
        }
    }

    PopupMenu {
        id: menu

        parent: root
        width: 268
        x: root.width - width
        y: root.height + 8
        anchorX: (menu.width - root.width / 2) / menu.width
        itemHeight: 40
        actions: [
            { "icon": "history", "text": qsTr("History") },
            { "icon": "logout", "text": qsTr("Sign out"), "destructive": true }
        ]

        header: Component {
            Item {
                implicitHeight: 44

                Artwork {
                    id: avatar

                    anchors.left: parent.left
                    anchors.leftMargin: menu.itemPadding
                    anchors.verticalCenter: parent.verticalCenter
                    width: 44
                    height: 44
                    visible: Account.artId.length > 0
                    artId: Account.artId
                    rounding: Theme.rounding.full
                }

                Rectangle {
                    anchors.fill: avatar
                    radius: Theme.rounding.full
                    color: Theme.colLayer4
                    visible: !avatar.visible

                    Sym {
                        anchors.centerIn: parent
                        text: "account_circle"
                        iconSize: 26
                        color: Theme.colOnSurfaceVariant
                    }
                }

                Column {
                    anchors.left: avatar.right
                    anchors.leftMargin: 12
                    anchors.right: parent.right
                    anchors.rightMargin: menu.itemPadding
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 2

                    StyledText {
                        width: parent.width
                        text: root.label
                        title: true
                        elide: Text.ElideRight
                    }

                    StyledText {
                        width: parent.width
                        visible: Account.handle.length > 0
                        text: Account.handle
                        elide: Text.ElideRight
                        font.pixelSize: Theme.font.smaller
                        color: Theme.colOnSurfaceVariant
                    }
                }
            }
        }

        onTriggered: index => {
            if (index === 0)
                root.historyRequested();
            else
                Account.signOut();
        }
    }
}
