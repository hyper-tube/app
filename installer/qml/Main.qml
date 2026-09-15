import QtQuick
import QtQuick.Window
import HtMusic
import HtMusic.Setup

Window {
    id: root

    readonly property int pageWelcome: 0
    readonly property int pageUninstall: 1
    readonly property int pageLicense: 2
    readonly property int pageOptions: 3
    readonly property int pageProgress: 4
    readonly property int pageFinish: 5

    property int page: root.pageWelcome
    property real shift: 0
    property bool ready: false
    property bool removalOnly: false
    property int outcome: Setup.Outcome.Cancelled
    property bool blockersOpen: false
    property bool cancelOpen: false
    property bool failed: false
    property string complaint: ""

    readonly property int frameWidth: 880
    readonly property int frameHeight: 600
    readonly property real pageTop: WindowChrome.barHeight
    readonly property real pageHeight: root.height - root.pageTop

    function begin() {
        Session.refreshBlockers();
        if (Session.blockingProcesses.length > 0) {
            root.blockersOpen = true;
            return;
        }
        if (Session.start())
            root.go(root.pageProgress);
    }

    function go(next) {
        root.page = next;
    }

    width: root.frameWidth
    height: root.frameHeight
    minimumWidth: root.frameWidth
    maximumWidth: root.frameWidth
    minimumHeight: root.frameHeight
    maximumHeight: root.frameHeight
    visible: false
    color: Theme.colLayer0
    flags: WindowChrome.windowFlags
    title: Session.productName

    onPageChanged: root.shift = root.page

    Behavior on shift {
        enabled: root.ready

        NumberAnimation {
            duration: Theme.duration.sheetEnter
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.emphasized
        }
    }

    Item {
        id: viewport

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: root.pageTop
        anchors.bottom: parent.bottom
        clip: true

        WelcomePage {
            width: viewport.width
            height: viewport.height
            x: (root.pageWelcome - root.shift) * viewport.width
            opacity: 1 - Math.min(1, Math.abs(root.pageWelcome - root.shift))
            visible: opacity > 0
            onCustomize: root.go(root.pageLicense)
            onInstall: root.begin()
            onShowLicense: root.go(root.pageLicense)
            onRemove: {
                Session.mode = Setup.Mode.Uninstall;
                root.go(root.pageUninstall);
            }
        }

        UninstallPage {
            width: viewport.width
            height: viewport.height
            x: (root.pageUninstall - root.shift) * viewport.width
            opacity: 1 - Math.min(1, Math.abs(root.pageUninstall - root.shift))
            visible: opacity > 0
            onRemove: root.begin()
            onBack: {
                if (root.removalOnly) {
                    Session.finish(Setup.Outcome.Cancelled);
                    return;
                }
                Session.mode = Session.maintenanceMode;
                root.go(root.pageWelcome);
            }
        }

        LicensePage {
            width: viewport.width
            height: viewport.height
            x: (root.pageLicense - root.shift) * viewport.width
            opacity: 1 - Math.min(1, Math.abs(root.pageLicense - root.shift))
            visible: opacity > 0
            onBack: root.go(root.pageWelcome)
            onAgreed: root.go(root.pageOptions)
        }

        OptionsPage {
            width: viewport.width
            height: viewport.height
            x: (root.pageOptions - root.shift) * viewport.width
            opacity: 1 - Math.min(1, Math.abs(root.pageOptions - root.shift))
            visible: opacity > 0
            onBack: root.go(root.pageLicense)
            onInstall: root.begin()
        }

        ProgressPage {
            width: viewport.width
            height: viewport.height
            x: (root.pageProgress - root.shift) * viewport.width
            opacity: 1 - Math.min(1, Math.abs(root.pageProgress - root.shift))
            visible: opacity > 0
        }

        FinishPage {
            width: viewport.width
            height: viewport.height
            x: (root.pageFinish - root.shift) * viewport.width
            opacity: 1 - Math.min(1, Math.abs(root.pageFinish - root.shift))
            visible: opacity > 0
            failed: root.failed
            complaint: root.complaint
            outcome: root.outcome
        }
    }

    WindowTitleBar {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        maximizeButton: false
    }

    ModalSheet {
        anchors.fill: parent
        open: root.blockersOpen
        title: qsTr("Close the running application")
        sheetWidth: 620
        sheetHeight: blockersBody.implicitHeight + 110
        onCloseRequested: root.blockersOpen = false

        Column {
            id: blockersBody

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 24
            spacing: 20

            StyledText {
                width: parent.width
                wrapMode: Text.WordWrap
                text: qsTr("Save your work and close these applications to continue: %1")
                    .arg(Session.blockingProcesses.join(", "))
            }

            Row {
                spacing: 12

                PillButton {
                    text: qsTr("Check again")
                    onClicked: {
                        Session.refreshBlockers();
                        if (Session.blockingProcesses.length === 0) {
                            root.blockersOpen = false;
                            root.begin();
                        }
                    }
                }

                PillButton {
                    text: qsTr("Close applications")
                    toggled: true
                    onClicked: {
                        if (Session.closeBlockers()) {
                            root.blockersOpen = false;
                            root.begin();
                        }
                    }
                }
            }
        }
    }

    ModalSheet {
        anchors.fill: parent
        open: root.cancelOpen
        title: qsTr("Cancel installation?")
        sheetWidth: 570
        sheetHeight: cancelBody.implicitHeight + 110
        onCloseRequested: root.cancelOpen = false

        Column {
            id: cancelBody

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 24
            spacing: 20

            StyledText {
                width: parent.width
                wrapMode: Text.WordWrap
                text: qsTr("Setup will undo the changes made during this installation.")
            }

            PillButton {
                text: qsTr("Cancel installation")
                onClicked: {
                    root.cancelOpen = false;
                    Session.job.cancel();
                }
            }
        }
    }

    Connections {
        target: Session

        function onElevationDeclined() {
            root.go(root.pageOptions);
        }
    }

    onClosing: close => {
        if (Session.job.running) {
            close.accepted = false;
            if (Session.mode !== Setup.Mode.Uninstall)
                root.cancelOpen = true;
        } else {
            close.accepted = false;
            Session.finish(root.page === root.pageFinish ? root.outcome : Setup.Outcome.Cancelled);
        }
    }

    Component.onCompleted: {
        root.removalOnly = Session.mode === Setup.Mode.Uninstall;
        if (root.removalOnly)
            root.go(root.pageUninstall);
        root.ready = true;
        if (Session.automatic)
            Qt.callLater(root.begin);
    }

    Connections {
        target: Session.job

        function onFinished(outcome, message) {
            root.outcome = outcome;
            root.failed = outcome === Setup.Outcome.Failed;
            root.complaint = message;
            root.cancelOpen = false;
            root.go(root.pageFinish);
        }
    }
}
