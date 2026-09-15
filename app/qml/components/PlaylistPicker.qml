import QtQuick
import QtQuick.Controls
import HtMusic

ModalSheet {
    id: root

    property bool creating: false

    readonly property real listHeight: Math.min(360, Math.max(120, PlaylistTargets.count * 64))

    open: PlaylistTargets.open
    title: PlaylistTargets.trackCount > 1
        ? qsTr("Save %n songs to playlist", "", PlaylistTargets.trackCount)
        : qsTr("Save to playlist")
    sheetWidth: Math.min(480, root.width - 96)
    sheetHeight: Math.min(root.height - 64,
        root.headerHeight + root.listHeight + (root.creating ? 150 : 84))

    onOpenChanged: {
        if (!root.open)
            root.creating = false;
    }
    onCloseRequested: PlaylistTargets.dismiss()

    Column {
        anchors.fill: parent
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        anchors.bottomMargin: 16
        spacing: 12

        Item {
            width: parent.width
            height: root.creating ? newName.height + 46 : 46

            Behavior on height {
                NumberAnimation {
                    duration: Theme.duration.spatialFast
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Theme.curve.expressiveFastSpatial
                }
            }

            PillButton {
                id: startCreate

                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.top: parent.top
                visible: !root.creating
                text: qsTr("New playlist")
                icon: "add"
                iconSize: 20
                leadingPadding: 12
                onClicked: {
                    root.creating = true;
                    newName.focusInput();
                }
            }

            InputField {
                id: newName

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                anchors.top: parent.top
                visible: root.creating
                label: qsTr("New playlist")
                placeholder: qsTr("Playlist name")
                maximumLength: 150
                onAccepted: root.create()
            }

            Row {
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.bottom: parent.bottom
                visible: root.creating
                spacing: 8

                PillButton {
                    text: qsTr("Cancel")
                    ghost: true
                    onClicked: root.creating = false
                }

                PillButton {
                    text: qsTr("Create")
                    toggled: true
                    interactive: newName.text.trim().length > 0
                    opacity: interactive ? 1 : 0.4
                    onClicked: root.create()
                }
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: Theme.colOutlineVariant
        }

        Item {
            width: parent.width
            height: root.listHeight

            BusySpinner {
                anchors.centerIn: parent
                visible: PlaylistTargets.loading
            }

            EmptyState {
                anchors.centerIn: parent
                width: parent.width
                visible: !PlaylistTargets.loading && PlaylistTargets.count === 0
                    && PlaylistTargets.error.length === 0
                icon: "playlist_add"
                title: qsTr("No playlists yet")
                caption: qsTr("Create one above to save this here")
            }

            ErrorState {
                anchors.centerIn: parent
                width: parent.width - 20
                height: 52
                size: ErrorState.Inline
                shown: !PlaylistTargets.loading && PlaylistTargets.error.length > 0
                busy: Connectivity.checking
                icon: PlaylistTargets.unreachable ? "cloud_off" : "error"
                title: PlaylistTargets.unreachable ? qsTr("You're offline")
                    : qsTr("Could not load playlists")
                caption: PlaylistTargets.error
                onActionTriggered: PlaylistTargets.retry()
            }

            ListView {
                id: list

                anchors.fill: parent
                clip: true
                visible: !PlaylistTargets.loading && PlaylistTargets.count > 0
                spacing: 2
                boundsBehavior: Flickable.StopAtBounds
                acceptedButtons: Qt.NoButton
                model: PlaylistTargets

                ScrollBar.vertical: PageScrollBar {}

                delegate: RippleSurface {
                    id: option

                    required property string targetTitle
                    required property string targetSubtitle
                    required property string targetArtId
                    required property int index

                    width: list.width
                    implicitHeight: 62
                    rounding: Theme.rounding.small

                    Artwork {
                        id: cover

                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        width: 44
                        height: 44
                        artId: option.targetArtId
                        rounding: Theme.rounding.verysmall
                    }

                    Column {
                        anchors.left: cover.right
                        anchors.leftMargin: 14
                        anchors.right: parent.right
                        anchors.rightMargin: 14
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 2

                        StyledText {
                            width: parent.width
                            text: option.targetTitle
                            title: true
                            font.pixelSize: Theme.font.small
                            elide: Text.ElideRight
                        }

                        StyledText {
                            width: parent.width
                            visible: option.targetSubtitle.length > 0
                            text: option.targetSubtitle
                            font.pixelSize: Theme.font.smaller
                            color: Theme.colInactive
                            elide: Text.ElideRight
                        }
                    }

                    onClicked: PlaylistTargets.choose(option.index)
                }
            }

            ScrollWheel {
                parent: list
                target: list
            }
        }
    }

    function create() {
        if (newName.text.trim().length === 0)
            return;
        PlaylistTargets.createWith(newName.text.trim());
        newName.text = "";
        root.creating = false;
    }
}
