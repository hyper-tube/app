import QtQuick
import QtQuick.Controls
import HtMusic

PillButton {
    id: root

    required property BrowseModel page

    readonly property bool country: !!root.page && root.page.charts

    text: root.page && root.page.sortIndex >= 0
        ? root.page.sortOptions[root.page.sortIndex].title : root.country ? qsTr("Country") : qsTr("Sort")
    icon: root.country ? "public" : "sort"
    leadingPadding: 12
    interactive: !!root.page && !root.page.loading && !root.page.reordering
    Accessible.name: root.country ? qsTr("Country: %1").arg(text) : qsTr("Sort by: %1").arg(text)
    onClicked: menu.toggle()

    Popover {
        id: menu

        parent: root
        x: root.page && root.page.kind === "library" ? Math.min(0, root.width - width) : 0
        y: root.height + 6
        width: 252
        anchorX: (root.width / 2 - menu.x) / menu.width

        ListView {
            id: options

            width: menu.columnWidth
            implicitHeight: Math.min(352, contentHeight)
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            model: root.page ? root.page.sortOptions : []
            currentIndex: root.page ? root.page.sortIndex : -1

            ScrollBar.vertical: PageScrollBar {}

            delegate: RippleSurface {
                id: option

                required property var modelData
                required property int index

                width: options.width
                implicitHeight: 44
                rounding: Theme.rounding.small
                colBackground: option.modelData.selected ? Theme.colSecondaryContainer : "transparent"

                StyledText {
                    anchors.left: parent.left
                    anchors.leftMargin: 14
                    anchors.right: tick.left
                    anchors.rightMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    text: option.modelData.title
                    title: option.modelData.selected
                    color: option.modelData.selected ? Theme.colOnSecondaryContainer : Theme.colOnSurface
                    elide: Text.ElideRight
                }

                Sym {
                    id: tick

                    anchors.right: parent.right
                    anchors.rightMargin: 14
                    anchors.verticalCenter: parent.verticalCenter
                    text: "check"
                    iconSize: 20
                    opacity: option.modelData.selected ? 1 : 0
                    color: Theme.colOnSecondaryContainer
                }

                onClicked: {
                    menu.close();
                    root.page.applySort(option.index);
                }
            }
        }

        onOpened: options.positionViewAtIndex(options.currentIndex, ListView.Contain)
    }
}
