import QtQuick
import HtMusic

Item {
    id: root

    property bool minimizeButton: true
    property bool maximizeButton: true

    readonly property real markInset: WindowChrome.leadingInset + Theme.size.gutter

    implicitHeight: WindowChrome.barHeight

    Item {
        id: grab

        anchors.left: parent.left
        anchors.right: buttons.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        enabled: WindowChrome.controls

        DragHandler {
            id: sweep

            target: null
            onActiveChanged: {
                if (sweep.active)
                    WindowChrome.startMove();
            }
        }

        TapHandler {
            enabled: root.maximizeButton
            gesturePolicy: TapHandler.DragThreshold
            onDoubleTapped: WindowChrome.toggleMaximized()
        }

        TapHandler {
            acceptedButtons: Qt.RightButton
            onTapped: WindowChrome.showSystemMenu()
        }
    }

    Wordmark {
        id: mark

        anchors.left: parent.left
        anchors.leftMargin: root.markInset
        anchors.verticalCenter: parent.verticalCenter
        opacity: Window.active ? 1 : 0.6

        Behavior on opacity {
            NumberAnimation {
                duration: Theme.duration.fast
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveEffects
            }
        }
    }

    Row {
        id: buttons

        anchors.right: parent.right
        anchors.rightMargin: Theme.size.gutter
        y: WindowChrome.gripThickness
        spacing: 6
        visible: WindowChrome.controls

        WindowButton {
            visible: root.minimizeButton
            icon: "remove"
            iconSize: 16
            Accessible.name: qsTr("Minimize")
            onClicked: WindowChrome.minimize()
        }

        WindowButton {
            id: zoom

            visible: root.maximizeButton
            icon: WindowChrome.maximized ? "filter_none" : "crop_square"
            iconSize: 11
            Accessible.name: WindowChrome.maximized ? qsTr("Restore down") : qsTr("Maximize")
            systemHovered: WindowChrome.snapHovered
            systemPressed: WindowChrome.snapPressed
            onClicked: WindowChrome.toggleMaximized()
        }

        WindowButton {
            icon: "close"
            iconSize: 18
            Accessible.name: qsTr("Close")
            onClicked: WindowChrome.close()
        }
    }

    Binding {
        target: WindowChrome
        property: "snapRegion"
        value: Qt.rect(buttons.x + zoom.x, buttons.y + zoom.y, zoom.width, zoom.height)
        when: WindowChrome.controls && root.maximizeButton
        restoreMode: Binding.RestoreNone
    }
}
