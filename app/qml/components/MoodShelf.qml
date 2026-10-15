import QtQuick
import HtMusic

Column {
    id: root

    property string title: ""
    property var cells: []
    property bool moreAvailable: false

    readonly property int rows: 4
    readonly property real gap: Theme.spaced(8)
    readonly property real cellHeight: Theme.sized(52) + root.gap
    readonly property real cellWidth: Math.max(Theme.sized(180), Math.min(Theme.sized(260), root.width / 5)) + root.gap
    readonly property bool scrollable: grid.contentWidth > grid.width

    signal cellActivated(int index)
    signal moreRequested

    spacing: Theme.spaced(16)

    Item {
        width: parent.width
        implicitHeight: Math.max(header.implicitHeight, arrows.implicitHeight)
        height: implicitHeight

        SectionHeader {
            id: header

            anchors.left: parent.left
            anchors.right: arrows.left
            anchors.rightMargin: Theme.spaced(16)
            anchors.verticalCenter: parent.verticalCenter
            title: root.title
        }

        Row {
            id: arrows

            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            spacing: Theme.spaced(2)

            PillButton {
                anchors.verticalCenter: parent.verticalCenter
                visible: root.moreAvailable
                text: qsTr("See all")
                onClicked: root.moreRequested()
            }

            IconButton {
                anchors.verticalCenter: parent.verticalCenter
                visible: root.scrollable
                icon: "chevron_left"
                Accessible.name: qsTr("Scroll left")
                iconSize: Theme.sized(24)
                interactive: !grid.atXBeginning
                opacity: grid.atXBeginning ? 0.3 : 1
                onClicked: glide.scrollBy(-1)

                Behavior on opacity {
                    NumberAnimation {
                        duration: Theme.duration.fast
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: Theme.curve.expressiveEffects
                    }
                }
            }

            IconButton {
                anchors.verticalCenter: parent.verticalCenter
                visible: root.scrollable
                icon: "chevron_right"
                Accessible.name: qsTr("Scroll right")
                iconSize: Theme.sized(24)
                interactive: !grid.atXEnd
                opacity: grid.atXEnd ? 0.3 : 1
                onClicked: glide.scrollBy(1)

                Behavior on opacity {
                    NumberAnimation {
                        duration: Theme.duration.fast
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: Theme.curve.expressiveEffects
                    }
                }
            }
        }
    }

    GridView {
        id: grid

        width: root.width
        height: root.cellHeight * Math.min(root.rows, Math.max(1, root.cells.length))
        flow: GridView.FlowTopToBottom
        cellWidth: root.cellWidth
        cellHeight: root.cellHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        acceptedButtons: Qt.NoButton
        cacheBuffer: 400
        model: root.cells

        delegate: Item {
            required property var modelData
            required property int index

            width: grid.cellWidth
            height: grid.cellHeight

            MoodCard {
                width: parent.width - root.gap
                height: parent.height - root.gap
                title: modelData.title
                onClicked: root.cellActivated(index)
            }
        }
    }

    ScrollGlide {
        id: glide

        target: grid
        step: Math.max(1, Math.floor(grid.width / root.cellWidth)) * root.cellWidth
    }
}
