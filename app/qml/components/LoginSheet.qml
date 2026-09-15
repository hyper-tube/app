import QtQuick
import QtWebEngine
import HtMusic

Item {
    id: root

    property bool open: false

    signal closeRequested

    visible: scrim.opacity > 0

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
        width: Math.min(560, parent.width - 96)
        height: Math.min(760, parent.height - 64)
        y: (parent.height - height) / 2 + 24 * (1 - reveal)
        opacity: reveal
        scale: 0.96 + 0.04 * reveal

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

        Item {
            id: header

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 60

            StyledText {
                anchors.left: parent.left
                anchors.leftMargin: 24
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Sign in to YouTube Music")
                title: true
                font.pixelSize: Theme.font.large
            }

            IconButton {
                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                icon: "close"
                Accessible.name: qsTr("Close")
                onClicked: root.closeRequested()
            }
        }

        Item {
            id: viewClip

            anchors.top: header.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 12
            anchors.topMargin: 0
            layer.enabled: true
            layer.effect: RoundedMask {
                rounding: Theme.rounding.normal
            }

            Rectangle {
                anchors.fill: parent
                color: Theme.colLayer0
            }

            Loader {
                id: viewLoader

                anchors.fill: parent
                active: root.open

                sourceComponent: WebEngineView {
                    profile: WebLogin.profile
                    url: WebLogin.signInUrl
                    backgroundColor: Theme.colLayer0

                    settings.javascriptCanOpenWindows: false

                    onLoadingChanged: request => {
                        if (request.status === WebEngineView.LoadSucceededStatus)
                            WebLogin.follow(request.url);
                    }
                }
            }
        }

        Column {
            anchors.centerIn: viewClip
            visible: !viewLoader.active || WebLogin.harvesting
            spacing: 12

            BusySpinner {
                anchors.horizontalCenter: parent.horizontalCenter
            }

            StyledText {
                visible: WebLogin.harvesting
                text: qsTr("Finishing sign-in...")
                color: Theme.colOnSurfaceVariant
            }
        }
    }

    Loader {
        width: 1
        height: 1
        opacity: 0
        clip: true
        active: Account.renewing

        sourceComponent: WebEngineView {
            width: 1024
            height: 768
            profile: WebLogin.profile
            url: WebLogin.landingUrl

            onLoadingChanged: request => {
                if (request.status === WebEngineView.LoadSucceededStatus)
                    WebLogin.follow(request.url);
            }
        }
    }

    Connections {
        target: WebLogin

        function onHarvested(credentialed) {
            if (credentialed && root.open)
                root.closeRequested();
        }
    }
}
