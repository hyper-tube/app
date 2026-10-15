import QtQuick
import HtMusic

RippleSurface {
    id: root

    property string title: ""
    property string icon: "explore"

    implicitWidth: Theme.sized(240)
    implicitHeight: Theme.sized(96)
    rounding: Theme.shape.card
    colBackground: Theme.colLayer2

    Rectangle {
        id: badge

        anchors.left: parent.left
        anchors.leftMargin: Theme.spaced(18)
        anchors.verticalCenter: parent.verticalCenter
        width: Theme.sized(48)
        height: Theme.sized(48)
        radius: Theme.shape.plate
        color: Theme.colPrimaryContainer

        Sym {
            anchors.centerIn: parent
            text: root.icon
            iconSize: Theme.font.huge
            fill: 1
            color: Theme.colOnPrimaryContainer
        }
    }

    StyledText {
        anchors.left: badge.right
        anchors.leftMargin: Theme.spaced(16)
        anchors.right: chevron.left
        anchors.rightMargin: Theme.spaced(8)
        anchors.verticalCenter: parent.verticalCenter
        text: root.title
        title: true
        font.pixelSize: Theme.font.large
        elide: Text.ElideRight
    }

    Sym {
        id: chevron

        anchors.right: parent.right
        anchors.rightMargin: Theme.spaced(16)
        anchors.verticalCenter: parent.verticalCenter
        text: "chevron_right"
        iconSize: Theme.font.larger
        color: Theme.colOnSurfaceVariant

        transform: Translate {
            x: root.hovered ? 0 : -4

            Behavior on x {
                NumberAnimation {
                    duration: Theme.duration.spatialFast
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Theme.curve.expressiveFastSpatial
                }
            }
        }
    }
}
