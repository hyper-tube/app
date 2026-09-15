import QtQuick
import QtQuick.Controls
import HtMusic

Rectangle {
    id: root

    property alias text: input.text
    property bool suggestionsEnabled: false
    property bool glass: false
    property int selectedEntry: -1
    property string icon: "search"
    property string placeholder: qsTr("Search songs, albums, artists")
    property real textPhase: 0

    readonly property bool recalling: input.text.trim().length === 0
    readonly property var entries: !root.suggestionsEnabled ? []
        : root.recalling ? SearchHistory.queries : suggestions.suggestions
    readonly property real expandedHeight: height + (panel.visible ? panel.height + 8 : 0)
    readonly property real textPresence: 1 - Math.abs(root.textPhase)

    signal accepted(string query)
    signal editingFinished

    function clearFocus() {
        input.focus = false;
        root.editingFinished();
    }

    function submit(query) {
        if (query.trim().length === 0)
            return;
        input.text = query;
        root.clearFocus();
        root.accepted(query);
    }

    function focusInput() {
        input.forceActiveFocus();
    }

    function follow(query) {
        if (input.activeFocus || query === input.text.trim()) {
            retarget.stop();
            root.textPhase = 0;
            return;
        }
        retarget.query = query;
        retarget.restart();
    }

    implicitHeight: 46
    radius: Theme.rounding.full
    color: root.glass ? ColorUtils.withAlpha(Theme.colLayer2, 0.55)
        : focusRing.visible ? Theme.colLayer2 : Theme.colLayer1
    border.width: 1
    border.color: input.activeFocus ? Theme.colPrimary : Theme.colOutlineVariant

    Behavior on color {
        ColorAnimation {
            duration: Theme.duration.fast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveEffects
        }
    }

    Behavior on border.color {
        ColorAnimation {
            duration: Theme.duration.fast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveEffects
        }
    }

    Item {
        id: focusRing

        visible: input.activeFocus || hover.hovered
    }

    Sym {
        id: glyph

        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        visible: root.icon.length > 0
        width: visible ? glyph.iconSize : 0
        text: root.icon
        iconSize: Theme.font.larger
        color: input.activeFocus ? Theme.colPrimary : Theme.colOnSurfaceVariant
    }

    TextInput {
        id: input

        anchors.left: glyph.right
        anchors.leftMargin: 12
        anchors.right: clear.left
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        verticalAlignment: TextInput.AlignVCenter
        opacity: root.textPresence
        color: Theme.colOnSurface
        selectionColor: Theme.colPrimary
        selectedTextColor: Theme.colOnPrimary
        selectByMouse: true
        clip: true

        font {
            family: Theme.font.main
            pixelSize: Theme.font.small
            variableAxes: Theme.font.axes
        }

        transform: Translate {
            x: root.textPhase * 8
        }

        onTextEdited: root.selectedEntry = -1
        onActiveFocusChanged: {
            if (!activeFocus)
                return;
            retarget.stop();
            root.textPhase = 0;
        }
        onAccepted: root.submit(root.selectedEntry >= 0
            ? root.entries[root.selectedEntry] : text)

        Keys.onDownPressed: root.selectedEntry = Math.min(root.entries.length - 1, root.selectedEntry + 1)
        Keys.onUpPressed: root.selectedEntry = Math.max(-1, root.selectedEntry - 1)
        Keys.onEscapePressed: event => {
            event.accepted = root.suggestionsEnabled;
            if (root.suggestionsEnabled)
                root.clearFocus();
        }

        StyledText {
            anchors.verticalCenter: parent.verticalCenter
            visible: input.text.length === 0
            text: root.placeholder
            color: Theme.colInactive
        }
    }

    IconButton {
        id: clear

        anchors.right: parent.right
        anchors.rightMargin: 4
        anchors.verticalCenter: parent.verticalCenter
        visible: input.text.length > 0
        opacity: root.textPresence
        scale: 0.6 + 0.4 * root.textPresence
        icon: "close"
        Accessible.name: qsTr("Clear search")
        iconSize: Theme.font.normal
        diameter: 36
        onClicked: input.clear()
    }

    SearchSuggestions {
        id: suggestions

        query: root.suggestionsEnabled && input.activeFocus ? input.text : ""
    }

    Popover {
        id: panel

        parent: root
        y: root.height + 8
        width: root.width
        focus: false
        closePolicy: Popup.NoAutoClose
        visible: root.suggestionsEnabled && input.activeFocus && root.entries.length > 0

        Repeater {
            model: root.entries

            delegate: RippleSurface {
                id: entry

                required property string modelData
                required property int index

                width: panel.columnWidth
                implicitHeight: 44
                rounding: panel.rounding - panel.inset
                colBackground: root.selectedEntry === entry.index
                    ? Theme.colSecondaryContainer : "transparent"

                Sym {
                    id: cue

                    anchors.left: parent.left
                    anchors.leftMargin: 14
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.recalling ? "history" : "search"
                    iconSize: Theme.font.normal
                    color: Theme.colOnSurfaceVariant
                }

                StyledText {
                    anchors.left: cue.right
                    anchors.leftMargin: 12
                    anchors.right: forget.left
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    text: entry.modelData
                    font.pixelSize: Theme.font.small
                    elide: Text.ElideRight
                }

                IconButton {
                    id: forget

                    anchors.right: parent.right
                    anchors.rightMargin: 6
                    anchors.verticalCenter: parent.verticalCenter
                    visible: root.recalling
                    icon: "close"
                    Accessible.name: qsTr("Remove from search history")
                    iconSize: Theme.font.normal
                    diameter: visible ? 32 : 0
                    onClicked: SearchHistory.forget(entry.modelData)
                }

                onClicked: root.submit(entry.modelData)
            }
        }
    }

    HoverHandler {
        id: hover

        cursorShape: Qt.IBeamCursor
    }

    TapHandler {
        onSingleTapped: {
            if (!input.activeFocus)
                root.focusInput();
        }
    }

    TapHandler {
        parent: root.Window.contentItem
        enabled: input.activeFocus

        onPressedChanged: {
            if (!pressed)
                return;
            const local = root.mapFromItem(null, point.scenePosition.x, point.scenePosition.y);
            if (local.x < 0 || local.x > root.width || local.y < 0 || local.y > root.expandedHeight)
                root.clearFocus();
        }
    }

    SequentialAnimation {
        id: retarget

        property string query: ""

        NumberAnimation {
            target: root
            property: "textPhase"
            to: -1
            duration: Theme.duration.exit
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.emphasizedAccel
        }

        ScriptAction {
            script: {
                input.text = retarget.query;
                root.textPhase = 1;
            }
        }

        NumberAnimation {
            target: root
            property: "textPhase"
            to: 0
            duration: Theme.duration.enter
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.emphasizedDecel
        }
    }

    onEntriesChanged: root.selectedEntry = -1
}
