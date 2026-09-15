import QtQuick
import HtMusic

ModalSheet {
    id: root

    property string playlistId: ""
    property string originalName: ""

    readonly property var privacyOptions: [
        { "value": "PUBLIC", "label": "Public", "caption": "Anyone can search for and view", "icon": "public" },
        { "value": "UNLISTED", "label": "Unlisted", "caption": "Anyone with the link can view", "icon": "link" },
        { "value": "PRIVATE", "label": "Private", "caption": "Only you can view", "icon": "lock" }
    ]
    readonly property bool dirty: name.text.trim().length > 0
        && (name.text.trim() !== root.originalName || description.text !== root.savedDescription
            || privacy.value !== root.savedPrivacy)

    property string savedDescription: ""
    property string savedPrivacy: "PRIVATE"

    title: qsTr("Edit playlist")
    sheetWidth: Math.min(560, root.width - 96)
    sheetHeight: Math.min(root.height - 64, root.headerHeight + form.implicitHeight + 40)

    function load(entry) {
        root.playlistId = entry.playlistTarget;
        root.originalName = entry.title;
        root.savedDescription = entry.description;
        root.savedPrivacy = entry.privacy.length > 0 ? entry.privacy : "PRIVATE";
        name.text = entry.title;
        description.text = entry.description;
        privacy.value = root.savedPrivacy;
        root.open = true;
    }

    Column {
        id: form

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: 24
        anchors.rightMargin: 24
        anchors.topMargin: 12
        spacing: 24

        InputField {
            id: name

            width: parent.width
            label: qsTr("Title")
            placeholder: qsTr("Playlist name")
            maximumLength: 150
            onAccepted: root.commit()
        }

        InputField {
            id: description

            width: parent.width
            label: qsTr("Description")
            placeholder: qsTr("Say something about this playlist")
            lines: 4
            maximumLength: 5000
        }

        SelectField {
            id: privacy

            width: parent.width
            label: qsTr("Privacy")
            options: root.privacyOptions
            value: root.savedPrivacy
            onPicked: chosen => privacy.value = chosen
        }

        Row {
            anchors.right: parent.right
            spacing: 8

            PillButton {
                text: qsTr("Cancel")
                ghost: true
                onClicked: root.closeRequested()
            }

            PillButton {
                text: qsTr("Save")
                icon: "check"
                toggled: true
                interactive: root.dirty
                opacity: interactive ? 1 : 0.4
                onClicked: root.commit()
            }
        }

        Item {
            width: parent.width
            height: 4
        }
    }

    function commit() {
        if (!root.dirty)
            return;
        LibraryActions.editPlaylist(root.playlistId, name.text.trim(), description.text,
                                    privacy.value);
        root.closeRequested();
    }
}
