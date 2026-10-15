import QtQuick
import QtQuick.Controls
import HtMusic

ModalSheet {
    id: root

    property bool creating: false

    readonly property real listHeight: Math.min(Theme.sized(360), Math.max(Theme.sized(120), PlaylistTargets.count * Theme.sized(64)))

    open: PlaylistTargets.open
    title: PlaylistTargets.trackCount > 1
        ? qsTr("Save %n songs to playlist", "", PlaylistTargets.trackCount)
        : qsTr("Save to playlist")
    sheetWidth: Math.min(Theme.sized(480), root.width - Theme.spaced(96))
    sheetHeight: Math.min(root.height - Theme.spaced(64),
        root.headerHeight + root.listHeight + (root.creating ? Theme.sized(150) : Theme.sized(84)))

    onOpenChanged: {
        if (!root.open)
            root.creating = false;
    }
    onCloseRequested: PlaylistTargets.dismiss()

    Column {
        anchors.fill: parent
        anchors.leftMargin: Theme.spaced(14)
        anchors.rightMargin: Theme.spaced(14)
        anchors.bottomMargin: Theme.spaced(16)
        spacing: Theme.spaced(12)

        Item {
            width: parent.width
            height: root.creating ? newName.height + Theme.sized(46) : Theme.sized(46)

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
                anchors.leftMargin: Theme.spaced(10)
                anchors.top: parent.top
                visible: !root.creating
                text: qsTr("New playlist")
                icon: "add"
                iconSize: Theme.sized(20)
                leadingPadding: Theme.spaced(12)
                onClicked: {
                    root.creating = true;
                    newName.focusInput();
                }
            }

            InputField {
                id: newName

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: Theme.spaced(10)
                anchors.rightMargin: Theme.spaced(10)
                anchors.top: parent.top
                visible: root.creating
                label: qsTr("New playlist")
                placeholder: qsTr("Playlist name")
                maximumLength: 150
                onAccepted: root.create()
            }

            Row {
                anchors.right: parent.right
                anchors.rightMargin: Theme.spaced(10)
                anchors.bottom: parent.bottom
                visible: root.creating
                spacing: Theme.spaced(8)

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
                width: parent.width - Theme.spaced(20)
                height: Theme.sized(52)
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
                spacing: Theme.spaced(2)
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
                    implicitHeight: Theme.sized(62)
                    rounding: Theme.shape.row

                    Artwork {
                        id: cover

                        anchors.left: parent.left
                        anchors.leftMargin: Theme.spaced(10)
                        anchors.verticalCenter: parent.verticalCenter
                        width: Theme.sized(44)
                        height: Theme.sized(44)
                        artId: option.targetArtId
                        rounding: Theme.shape.thumbnail
                    }

                    Column {
                        anchors.left: cover.right
                        anchors.leftMargin: Theme.spaced(14)
                        anchors.right: parent.right
                        anchors.rightMargin: Theme.spaced(14)
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: Theme.spaced(2)

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
