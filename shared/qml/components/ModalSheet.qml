import QtQuick
import QtQuick.Controls
import HtMusic

Item {
    id: root

    property bool open: false
    property string title: ""
    property real sheetWidth: Math.min(920, root.width - 96)
    property real sheetHeight: Math.min(root.height - 64, Math.max(340, root.contentHeight + 62))
    property real contentHeight: body.implicitHeight

    readonly property alias body: body
    readonly property alias headerContent: headerContent.data
    readonly property real headerHeight: header.height

    default property alias content: body.data

    signal closeRequested

    visible: popup.visible

    Popup {
        id: popup

        parent: Overlay.overlay
        width: parent ? parent.width : 0
        height: parent ? parent.height : 0
        padding: 0
        z: 100
        modal: true
        dim: false
        focus: true
        visible: root.open || scrim.opacity > 0
        closePolicy: Popup.NoAutoClose
        background: null

        contentItem: Item {
            focus: true

            Keys.onEscapePressed: root.closeRequested()

            Rectangle {
                id: scrim

                anchors.fill: parent
                color: Theme.colScrim
                opacity: root.open ? 0.55 : 0

                Behavior on opacity {
                    NumberAnimation {
                        duration: Theme.duration.fast
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: Theme.curve.expressiveEffects
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.AllButtons
                    hoverEnabled: true
                    onWheel: event => event.accepted = true
                    onClicked: root.closeRequested()
                }
            }

            Item {
                id: sheet

                property real reveal: root.open ? 1 : 0

                anchors.horizontalCenter: parent.horizontalCenter
                width: root.sheetWidth
                height: root.sheetHeight
                y: (parent.height - height) / 2 + 24 * (1 - reveal)
                opacity: reveal
                scale: 0.96 + 0.04 * reveal

                Behavior on height {
                    NumberAnimation {
                        duration: Theme.duration.spatial
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: Theme.curve.expressiveDefaultSpatial
                    }
                }

                Behavior on reveal {
                    NumberAnimation {
                        duration: Theme.duration.enter
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: Theme.curve.emphasizedDecel
                    }
                }

                Rectangle {
                    anchors.fill: parent
                    radius: Theme.rounding.large
                    color: Theme.colLayer1
                    border.width: 1
                    border.color: Theme.colOutlineVariant
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.AllButtons
                    onWheel: event => event.accepted = true
                }

                Item {
                    id: header

                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 62

                    StyledText {
                        anchors.left: parent.left
                        anchors.leftMargin: 24
                        anchors.right: dismiss.left
                        anchors.rightMargin: 12
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.title
                        title: true
                        font.pixelSize: Theme.font.huge
                        elide: Text.ElideRight
                    }

                    Item {
                        id: headerContent

                        anchors.left: parent.left
                        anchors.right: dismiss.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                    }

                    IconButton {
                        id: dismiss

                        anchors.right: parent.right
                        anchors.rightMargin: 14
                        anchors.verticalCenter: parent.verticalCenter
                        icon: "close"
                        Accessible.name: qsTr("Close")
                        onClicked: root.closeRequested()
                    }
                }

                Item {
                    id: body

                    anchors.top: header.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                }
            }
        }
    }
}
