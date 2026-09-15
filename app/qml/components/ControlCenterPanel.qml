import QtQuick
import QtQuick.Controls
import HtMusic

Popover {
    id: root

    readonly property real listCeiling: 420
    readonly property bool empty: ControlCenter.count === 0

    property real emptyReveal: root.empty ? 1 : 0

    width: 396
    gap: 0

    Behavior on emptyReveal {
        NumberAnimation {
            duration: Theme.duration.spatialFast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveFastSpatial
        }
    }

    Item {
        width: root.columnWidth
        implicitHeight: 44

        StyledText {
            anchors.left: parent.left
            anchors.leftMargin: 14
            anchors.right: sweep.left
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Control center")
            title: true
            elide: Text.ElideRight
        }

        IconButton {
            id: sweep

            anchors.right: parent.right
            anchors.rightMargin: 4
            anchors.verticalCenter: parent.verticalCenter
            icon: "clear_all"
            iconSize: 20
            diameter: 36
            interactive: ControlCenter.clearable
            opacity: ControlCenter.clearable ? 1 : 0.3
            Accessible.name: qsTr("Dismiss all notifications")
            onClicked: ControlCenter.dismissAll()
        }
    }

    Item {
        width: root.columnWidth
        implicitHeight: cards.children.length > 0 ? cards.implicitHeight + 8 : 0

        Column {
            id: cards

            x: 4
            width: parent.width - 8
            spacing: 8

            Repeater {
                model: PluginRegistry.cards

                delegate: Loader {
                    required property string modelData

                    width: cards.width
                    source: modelData
                }
            }
        }
    }

    Flickable {
        id: scroller

        width: root.columnWidth
        implicitHeight: Math.min(stack.implicitHeight, root.listCeiling)
        contentHeight: stack.implicitHeight
        contentWidth: width
        clip: true
        interactive: stack.implicitHeight > height
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: PageScrollBar {}

        Column {
            id: stack

            width: scroller.width
            spacing: 0

            Repeater {
                model: ControlCenter

                delegate: NotificationRow {
                    width: parent.width
                }
            }
        }
    }

    Item {
        width: root.columnWidth
        implicitHeight: 148 * root.emptyReveal
        clip: true

        EmptyState {
            width: parent.width
            height: 148
            opacity: root.emptyReveal
            scale: 0.9 + 0.1 * root.emptyReveal
            icon: "notifications_off"
            title: qsTr("All clear")
            caption: qsTr("Downloads and announcements land here")
        }
    }

    onClosed: ControlCenter.markRead()
}
