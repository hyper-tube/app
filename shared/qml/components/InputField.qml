import QtQuick
import HtMusic

Item {
    id: root

    property alias text: input.text
    property string label: ""
    property string placeholder: ""
    property int lines: 1
    property int maximumLength: 0

    readonly property bool focused: input.activeFocus
    readonly property real lineHeight: 22

    signal accepted

    function focusInput() {
        input.forceActiveFocus();
    }

    function clearFocus() {
        input.focus = false;
    }

    implicitWidth: 260
    implicitHeight: 15 + Math.max(root.lineHeight, root.lines * root.lineHeight) + 15

    Rectangle {
        anchors.fill: parent
        radius: Theme.rounding.small
        color: "transparent"
        border.width: root.focused ? 2 : 1
        border.color: root.focused ? Theme.colPrimary : Theme.colOutlineVariant

        Behavior on border.color {
            ColorAnimation {
                duration: Theme.duration.fast
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveEffects
            }
        }
    }

    Rectangle {
        x: 12
        y: -7
        width: caption.implicitWidth + 8
        height: 14
        visible: root.label.length > 0
        color: Theme.colLayer1

        StyledText {
            id: caption

            anchors.centerIn: parent
            text: root.label
            font.pixelSize: Theme.font.smallest
            color: root.focused ? Theme.colPrimary : Theme.colOnSurfaceVariant
        }
    }

    TextEdit {
        id: input

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        anchors.verticalCenter: parent.verticalCenter
        height: Math.max(root.lineHeight, root.lines * root.lineHeight)
        color: Theme.colOnSurface
        selectionColor: Theme.colPrimary
        selectedTextColor: Theme.colOnPrimary
        selectByMouse: true
        wrapMode: root.lines > 1 ? TextEdit.Wrap : TextEdit.NoWrap
        clip: true

        font {
            family: Theme.font.main
            pixelSize: Theme.font.small
            variableAxes: Theme.font.axes
        }

        Keys.onReturnPressed: event => {
            if (root.lines > 1) {
                event.accepted = false;
                return;
            }
            event.accepted = true;
            root.accepted();
        }

        onTextChanged: {
            if (root.maximumLength > 0 && input.text.length > root.maximumLength)
                input.text = input.text.slice(0, root.maximumLength);
        }

        StyledText {
            anchors.left: parent.left
            anchors.top: parent.top
            visible: input.text.length === 0
            text: root.placeholder
            font.pixelSize: Theme.font.small
            color: Theme.colInactive
        }
    }

    HoverHandler {
        cursorShape: Qt.IBeamCursor
    }

    TapHandler {
        onSingleTapped: root.focusInput()
    }
}
