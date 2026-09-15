import QtQuick
import HtMusic

Item {
    id: root

    property bool glass: false
    property bool welcoming: false
    property real reveal: 0
    property real welcome: root.welcoming && Connectivity.online ? 1 : 0
    property real probing: Connectivity.checking && !Connectivity.online ? 1 : 0

    readonly property bool shown: !Connectivity.online || root.welcoming
    readonly property real labelWidth: offlineLabel.implicitWidth
        + (onlineLabel.implicitWidth - offlineLabel.implicitWidth) * root.welcome
    readonly property real fullWidth: 14 + Theme.font.larger + 8 + root.labelWidth + 16

    implicitWidth: root.fullWidth * Math.max(0, root.reveal)
    implicitHeight: 40
    visible: root.reveal > 0
    opacity: Math.min(1, root.reveal)
    clip: true

    Behavior on welcome {
        NumberAnimation {
            duration: Theme.duration.spatialFast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveEffects
        }
    }

    Behavior on probing {
        NumberAnimation {
            duration: Theme.duration.fast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveEffects
        }
    }

    RippleSurface {
        anchors.right: parent.right
        width: root.fullWidth
        height: parent.height
        rounding: Theme.rounding.full
        interactive: !Connectivity.online
        tooltip: qsTr("You're offline. Select to check the connection again.")
        Accessible.role: Accessible.Button
        Accessible.name: Connectivity.online ? qsTr("Back online") : qsTr("Offline")
        colBackground: root.glass ? ColorUtils.withAlpha(Theme.colLayer2, 0.5)
            : Connectivity.online ? Theme.colSecondaryContainer : Theme.colLayer2
        colState: Theme.colOnSurface

        Item {
            id: glyphs

            x: 14
            anchors.verticalCenter: parent.verticalCenter
            width: Theme.font.larger
            height: Theme.font.larger

            Sym {
                anchors.centerIn: parent
                text: "cloud_off"
                iconSize: Theme.font.larger
                color: Theme.colOnSurfaceVariant
                opacity: (1 - root.welcome) * (1 - root.probing)
                scale: 1 - 0.3 * Math.max(root.welcome, root.probing)
            }

            Sym {
                anchors.centerIn: parent
                text: "cloud_done"
                iconSize: Theme.font.larger
                fill: 1
                color: Theme.colOnSecondaryContainer
                opacity: root.welcome
                scale: 0.6 + 0.4 * root.welcome
            }

            BusySpinner {
                anchors.centerIn: parent
                width: 16
                height: 16
                visible: root.probing > 0
                opacity: root.probing * (1 - root.welcome)
                colArc: Theme.colOnSurfaceVariant
            }
        }

        Item {
            x: glyphs.x + glyphs.width + 8
            anchors.verticalCenter: parent.verticalCenter
            width: root.labelWidth
            height: offlineLabel.implicitHeight

            StyledText {
                id: offlineLabel

                text: qsTr("Offline")
                font.pixelSize: Theme.font.smallie
                color: Theme.colOnSurfaceVariant
                opacity: 1 - root.welcome
            }

            StyledText {
                id: onlineLabel

                text: qsTr("Back online")
                font.pixelSize: Theme.font.smallie
                color: Theme.colOnSecondaryContainer
                opacity: root.welcome
            }
        }

        onClicked: Connectivity.check()
    }

    Timer {
        id: linger

        interval: 2600
        onTriggered: root.welcoming = false
    }

    Connections {
        target: Connectivity

        function onOnlineChanged() {
            root.welcoming = Connectivity.online;
            if (Connectivity.online)
                linger.restart();
            else
                linger.stop();
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
                duration: Theme.duration.spatial
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveDefaultSpatial
            }
        },
        Transition {
            from: "shown"

            NumberAnimation {
                property: "reveal"
                duration: Theme.duration.spatialFast
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.emphasizedAccel
            }
        }
    ]
}
