import QtQuick
import HtMusic
import HtMusic.Setup

Item {
    id: root

    property int outcome: Setup.Outcome.Succeeded
    property bool failed: false
    property string complaint: ""
    property real reveal: 0

    readonly property bool cancelled: root.outcome === Setup.Outcome.Cancelled
    readonly property bool removing: Session.mode === Setup.Mode.Uninstall
    readonly property bool settled: root.failed || root.cancelled

    readonly property string badge: root.cancelled ? "close"
        : root.failed ? "priority_high"
        : root.removing ? "upload"
        : "download_done"

    Column {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -30
        width: Math.min(parent.width - Theme.size.gutter * 6, 540)
        spacing: 20

        Item {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 88
            height: 88

            Rectangle {
                anchors.centerIn: parent
                width: 88
                height: 88
                radius: 44
                color: root.settled ? Theme.colLayer2 : Theme.colPrimaryContainer
                scale: 0.7 + 0.3 * root.reveal
                opacity: root.reveal
            }

            Sym {
                anchors.centerIn: parent
                text: root.badge
                iconSize: 44
                weight: Font.DemiBold
                grade: 100
                color: root.settled ? Theme.colError : Theme.colOnPrimaryContainer
                scale: 0.5 + 0.5 * root.reveal
                opacity: root.reveal
            }
        }

        StyledText {
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            text: root.cancelled ? qsTr("Installation cancelled") : root.failed
                ? qsTr("Something went wrong")
                : root.removing
                    ? qsTr("%1 has been removed").arg(Session.productName)
                    : qsTr("%1 is ready").arg(Session.productName)
            title: true
            font.pixelSize: Theme.font.display
        }

        StyledText {
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.pixelSize: Theme.font.normal
            color: root.failed ? Theme.colError : Theme.colOnSurfaceVariant
            text: root.cancelled ? qsTr("Your previous installation was kept.") : root.failed
                ? root.complaint
                : root.removing
                    ? (Session.removeUserData ? qsTr("Your settings, downloads and history were removed.")
                        : qsTr("Your settings and downloads were left where they are."))
                    : qsTr("Installed into %1").arg(Session.directory)
        }
    }

    Column {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: footer.top
        anchors.bottomMargin: 12
        width: parent.width - Theme.size.gutter * 4
        spacing: 10
        visible: root.failed

        StyledText {
            width: parent.width
            text: qsTr("Log: %1").arg(Session.logFile)
            wrapMode: Text.WrapAnywhere
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Theme.font.smaller
        }

        PillButton {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Copy details")
            onClicked: Session.copyDetails(root.complaint)
        }
    }

    RippleSurface {
        id: launch

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: footer.top
        anchors.bottomMargin: 20
        width: label.implicitWidth + tick.width + 34
        height: 40
        rounding: Theme.rounding.small
        visible: !root.failed && !root.cancelled && !root.removing
        onClicked: Session.launchWhenDone = !Session.launchWhenDone

        CheckMark {
            id: tick

            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            checked: Session.launchWhenDone
        }

        StyledText {
            id: label

            anchors.left: tick.right
            anchors.leftMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Open %1 now").arg(Session.productName)
        }
    }

    Item {
        id: footer

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 88

        PillButton {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Done")
            toggled: true
            horizontalPadding: 34
            onClicked: {
                if (!root.failed && !root.cancelled && !root.removing && Session.launchWhenDone)
                    Session.launchApplication();
                Session.finish(root.outcome);
            }
        }
    }

    onVisibleChanged: root.reveal = root.visible ? 1 : 0

    Behavior on reveal {
        NumberAnimation {
            duration: Theme.duration.spatial
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveDefaultSpatial
        }
    }
}
