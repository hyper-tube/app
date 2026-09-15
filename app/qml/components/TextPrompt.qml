import QtQuick
import QtQuick.Controls
import HtMusic

Item {
    id: root

    property bool open: false
    property string title: ""
    property string placeholder: ""
    property string confirm: qsTr("Create")
    property string message: ""
    property bool showInput: true

    readonly property bool modalVisible: popup.visible

    signal accepted(string value)
    signal dismissed

    Popup {
        id: popup

        property real reveal: 0

        opacity: reveal
        scale: 0.94 + 0.06 * reveal

        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(440, parent.width - 64)
        padding: 24
        z: 110
        modal: true
        focus: true
        visible: root.open
        closePolicy: Popup.CloseOnEscape

        background: Rectangle {
            radius: Theme.rounding.large
            color: Theme.colLayer2
            border.width: 1
            border.color: Theme.colOutlineVariant
        }

        Overlay.modal: Rectangle {
            color: ColorUtils.withAlpha(Theme.colScrim, 0.5)
        }

        contentItem: Column {
            spacing: 20

            Keys.onPressed: event => {
                if (event.key === Qt.Key_Escape)
                    root.dismissed();
                event.accepted = true;
            }

            StyledText {
                width: parent.width
                text: root.title
                title: true
                font.pixelSize: Theme.font.huge
            }

            StyledText {
                width: parent.width
                visible: root.message.length > 0
                text: root.message
                wrapMode: Text.WordWrap
                color: Theme.colOnSurfaceVariant
            }

            SearchBar {
                id: field

                width: parent.width
                visible: root.showInput
                icon: ""
                placeholder: root.placeholder
                onAccepted: value => root.accepted(value)
            }

            Row {
                anchors.right: parent.right
                spacing: 8

                PillButton {
                    text: qsTr("Cancel")
                    onClicked: root.dismissed()
                }

                PillButton {
                    text: root.confirm
                    toggled: true
                    interactive: !root.showInput || field.text.trim().length > 0
                    opacity: interactive ? 1 : 0.4
                    onClicked: root.accepted(field.text)
                }
            }
        }

        enter: Transition {
            NumberAnimation {
                target: popup
                property: "reveal"
                from: 0
                to: 1
                duration: Theme.duration.enter
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.emphasizedDecel
            }
        }

        exit: Transition {
            NumberAnimation {
                target: popup
                property: "reveal"
                to: 0
                duration: Theme.duration.exit
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.emphasizedAccel
            }
        }

        onOpened: {
            field.text = "";
            if (root.showInput)
                field.focusInput();
        }
        onClosed: root.open = false
    }
}
