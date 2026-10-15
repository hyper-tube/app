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
    leadingPadding: Theme.spaced(12)
    rounding: Theme.shape.chip
    interactive: !!root.page && !root.page.loading && !root.page.reordering
    Accessible.name: root.country ? qsTr("Country: %1").arg(text) : qsTr("Sort by: %1").arg(text)
    onClicked: menu.toggle()

    Popover {
        id: menu

        parent: root
        x: root.page && root.page.kind === "library" ? Math.min(0, root.width - width) : 0
        y: root.height + Theme.spaced(6)
        width: Theme.sized(252)
        anchorX: (root.width / 2 - menu.x) / menu.width

        ListView {
            id: options

            width: menu.columnWidth
            implicitHeight: Math.min(Theme.sized(352), contentHeight)
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
                implicitHeight: Theme.sized(44)
                rounding: Math.max(0, menu.rounding - menu.inset)
                colBackground: option.modelData.selected ? Theme.colSecondaryContainer : "transparent"

                StyledText {
                    anchors.left: parent.left
                    anchors.leftMargin: Theme.spaced(14)
                    anchors.right: tick.left
                    anchors.rightMargin: Theme.spaced(10)
                    anchors.verticalCenter: parent.verticalCenter
                    text: option.modelData.title
                    title: option.modelData.selected
                    color: option.modelData.selected ? Theme.colOnSecondaryContainer : Theme.colOnSurface
                    elide: Text.ElideRight
                }

                Sym {
                    id: tick

                    anchors.right: parent.right
                    anchors.rightMargin: Theme.spaced(14)
                    anchors.verticalCenter: parent.verticalCenter
                    text: "check"
                    iconSize: Theme.sized(20)
                    opacity: option.modelData.selected ? 1 : 0
                    color: Theme.colOnSecondaryContainer
                }

                onClicked: {
                    menu.close();
                    root.page.applySort(option.index);
                }
            }
        }

        ScrollWheel {
            parent: options
            target: options
        }

        onOpened: options.positionViewAtIndex(options.currentIndex, ListView.Contain)
    }
}
