import QtQuick
import HtMusic

Popover {
    id: root

    property var actions: []
    property Component header: null
    property real minimumWidth: 236
    property real itemHeight: 44

    readonly property real itemPadding: 14
    readonly property real itemGlyphSize: Theme.font.larger
    readonly property real itemRounding: root.rounding - root.inset
    readonly property bool divided: root.header !== null && root.actions.length > 0

    signal triggered(int index)

    width: Math.max(root.minimumWidth, labels.implicitWidth + root.itemGlyphSize
        + root.itemPadding * 3 + root.inset * 2)

    Column {
        id: labels

        visible: false

        Repeater {
            model: root.actions

            delegate: StyledText {
                required property var modelData

                text: modelData.text
            }
        }
    }

    Loader {
        width: root.columnWidth
        height: item ? item.implicitHeight : 0
        visible: item !== null
        sourceComponent: root.header
    }

    Item {
        x: -root.inset
        width: root.width
        height: 13
        visible: root.divided

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width
            height: 1
            color: Theme.colOutlineVariant
        }
    }

    Repeater {
        model: root.actions

        delegate: RippleSurface {
            id: entry

            required property var modelData
            required property int index

            readonly property bool destructive: entry.modelData.destructive === true

            width: root.columnWidth
            implicitHeight: root.itemHeight
            rounding: root.itemRounding
            colState: entry.destructive ? Theme.colError : Theme.colOnSurface

            Sym {
                id: glyph

                anchors.left: parent.left
                anchors.leftMargin: root.itemPadding
                anchors.verticalCenter: parent.verticalCenter
                fill: entry.modelData.active === true ? 1 : 0
                text: entry.modelData.icon
                iconSize: root.itemGlyphSize
                color: entry.destructive ? Theme.colError : Theme.colOnSurfaceVariant
            }

            StyledText {
                anchors.left: glyph.right
                anchors.leftMargin: root.itemPadding
                anchors.right: parent.right
                anchors.rightMargin: root.itemPadding
                anchors.verticalCenter: parent.verticalCenter
                text: entry.modelData.text
                color: entry.destructive ? Theme.colError : Theme.colOnSurface
                elide: Text.ElideRight
            }

            onClicked: {
                const chosen = entry.index;
                root.close();
                root.triggered(chosen);
            }
        }
    }
}
