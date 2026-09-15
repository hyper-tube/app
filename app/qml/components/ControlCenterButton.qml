import QtQuick
import HtMusic

Item {
    id: root

    readonly property bool busy: ControlCenter.status === ControlCenter.Busy
    readonly property bool alert: ControlCenter.status === ControlCenter.Alert
    readonly property real lift: 4

    property real busyReveal: root.busy ? 1 : 0
    property real alertReveal: root.alert ? 1 : 0
    property real barFill: ControlCenter.progress
    property real dotPop: 1

    implicitWidth: 40
    implicitHeight: 40

    Behavior on busyReveal {
        NumberAnimation {
            duration: Theme.duration.spatialFast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveFastSpatial
        }
    }

    Behavior on alertReveal {
        NumberAnimation {
            duration: Theme.duration.spatialFast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveFastSpatial
        }
    }

    Behavior on barFill {
        NumberAnimation {
            duration: Theme.duration.fast
            easing.type: Easing.OutCubic
        }
    }

    RippleSurface {
        id: surface

        anchors.fill: parent
        rounding: Theme.rounding.full
        tooltip: qsTr("Control center")
        Accessible.name: qsTr("Control center")

        Item {
            id: glyphs

            anchors.fill: parent
            scale: surface.pressed ? 0.86 : 1

            Behavior on scale {
                NumberAnimation {
                    duration: Theme.duration.spatialFast
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Theme.curve.expressiveFastSpatial
                }
            }

            Sym {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: -root.lift * root.busyReveal
                text: "tune"
                iconSize: Theme.font.larger
                color: Theme.colOnSurfaceVariant
                opacity: 1 - root.busyReveal
                scale: 1 - 0.2 * root.busyReveal
            }

            Sym {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: -root.lift * root.busyReveal
                text: "download"
                iconSize: Theme.font.larger
                color: Theme.colOnSurfaceVariant
                opacity: root.busyReveal
                scale: 0.8 + 0.2 * root.busyReveal
            }

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                y: parent.height / 2 + 7
                width: 18
                height: 3
                radius: height / 2
                color: ColorUtils.withAlpha(Theme.colOnSurfaceVariant, 0.28)
                opacity: root.busyReveal
                scale: 0.6 + 0.4 * root.busyReveal

                Rectangle {
                    width: parent.width * Math.max(0, Math.min(1, root.barFill))
                    height: parent.height
                    radius: parent.radius
                    color: Theme.colPrimary
                }
            }
        }

        onClicked: panel.toggle()
    }

    Rectangle {
        x: root.width / 2 + 7
        y: root.height / 2 - 15
        width: 8
        height: 8
        radius: width / 2
        color: Theme.colPrimary
        opacity: root.alertReveal
        scale: root.alertReveal * root.dotPop
    }

    SequentialAnimation {
        id: pop

        NumberAnimation {
            target: root
            property: "dotPop"
            to: 1.6
            duration: 140
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            target: root
            property: "dotPop"
            to: 1
            duration: Theme.duration.spatialFast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveFastSpatial
        }
    }

    Connections {
        target: ControlCenter

        function onArrived() {
            pop.restart();
        }
    }

    Connections {
        target: PluginRegistry

        function onPageRequested(id) {
            panel.close();
        }
    }

    Connections {
        target: Updater

        function onDetailsRequested() {
            panel.close();
        }
    }

    ControlCenterPanel {
        id: panel

        parent: root
        x: root.width - width
        y: root.height + 10
        anchorX: (panel.width - root.width / 2) / panel.width
    }
}
