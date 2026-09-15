import QtQuick
import HtMusic

Column {
    id: root

    required property var cells
    property string title: ""
    property string subtitle: ""
    property bool moreAvailable: false

    readonly property int rowsPerColumn: 4
    readonly property real columnWidth: Math.max(280, (root.width - 24) / 3)
    readonly property real stride: root.columnWidth + strip.spacing

    signal activated(int index)
    signal menuRequested(int index, Item source, real x, real y)
    signal moreRequested
    signal endReached

    spacing: 16

    Item {
        width: parent.width
        height: Math.max(heading.implicitHeight, arrows.implicitHeight)

        SectionHeader {
            id: heading

            anchors.left: parent.left
            anchors.right: arrows.left
            anchors.rightMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            title: root.title
            subtitle: root.subtitle
        }

        Row {
            id: arrows

            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2

            PillButton {
                anchors.verticalCenter: parent.verticalCenter
                visible: root.moreAvailable
                text: qsTr("See all")
                onClicked: root.moreRequested()
            }

            IconButton {
                icon: "chevron_left"
                Accessible.name: qsTr("Previous rankings")
                interactive: !strip.atXBeginning
                opacity: interactive ? 1 : 0.3
                onClicked: glide.scrollBy(-1)
            }

            IconButton {
                icon: "chevron_right"
                Accessible.name: qsTr("Next rankings")
                interactive: !strip.atXEnd
                opacity: interactive ? 1 : 0.3
                onClicked: glide.scrollBy(1)
            }
        }
    }

    ListView {
        id: strip

        width: parent.width
        height: Math.min(root.rowsPerColumn, root.cells.length) * 72
        orientation: ListView.Horizontal
        spacing: 12
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        acceptedButtons: Qt.NoButton
        model: Math.ceil(root.cells.length / root.rowsPerColumn)

        delegate: Column {
            id: group

            required property int index

            width: root.columnWidth
            spacing: 4

            Repeater {
                model: Math.min(root.rowsPerColumn, root.cells.length - group.index * root.rowsPerColumn)

                delegate: RankedRow {
                    id: row

                    required property int index

                    readonly property int itemIndex: group.index * root.rowsPerColumn + index

                    width: group.width
                    entry: root.cells[itemIndex]
                    onClicked: root.activated(itemIndex)
                    onMenuRequested: (x, y) => root.menuRequested(row.itemIndex, row, x, y)
                }
            }
        }

        onContentXChanged: {
            if (contentX + width + 320 >= originX + contentWidth)
                root.endReached();
        }
    }

    ScrollGlide {
        id: glide

        target: strip
        step: Math.max(1, Math.floor((strip.width + strip.spacing) / root.stride)) * root.stride
    }
}
