import QtQuick
import HtMusic

Item {
    id: root

    property string icon: "inbox"
    property string title: ""
    property string caption: ""
    property bool shown: true
    property real reveal: 0

    function stage(index) {
        return Math.max(0, Math.min(1, (root.reveal - index * 0.14) / 0.72));
    }

    implicitWidth: column.implicitWidth
    implicitHeight: column.implicitHeight
    visible: root.reveal > 0

    Column {
        id: column

        anchors.centerIn: parent
        spacing: 10

        Sym {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.icon
            iconSize: 44
            color: Theme.colInactive
            opacity: root.stage(0)
            scale: 0.6 + 0.4 * root.reveal
        }

        StyledText {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.title.length > 0
            text: root.title
            title: true
            font.pixelSize: Theme.font.large
            color: Theme.colOnSurfaceVariant
            opacity: root.stage(1)
            transform: Translate { y: 12 * (1 - root.stage(1)) }
        }

        StyledText {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.caption.length > 0
            text: root.caption
            font.pixelSize: Theme.font.smallie
            color: Theme.colInactive
            opacity: root.stage(2)
            transform: Translate { y: 12 * (1 - root.stage(2)) }
        }
    }

    states: State {
        name: "shown"
        when: root.shown

        PropertyChanges {
            target: root
            reveal: 1
        }
    }

    transitions: [
        Transition {
            to: "shown"

            NumberAnimation {
                property: "reveal"
                duration: Theme.duration.spatialSlow
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveDefaultSpatial
            }
        },
        Transition {
            from: "shown"

            NumberAnimation {
                property: "reveal"
                duration: Theme.duration.exit
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.emphasizedAccel
            }
        }
    ]
}
