import QtQuick
import QtQuick.Effects
import HtMusic

Column {
    id: root

    property int nextId: 0
    property var pending: []
    property bool downloadVisible: false

    readonly property bool downloadActive: Downloads.activeTitle.length > 0 && Downloads.activeForced

    onDownloadActiveChanged: {
        downloadDelay.stop();
        root.downloadVisible = false;
        if (root.downloadActive)
            downloadDelay.start();
    }

    Timer {
        id: downloadDelay

        interval: 1000
        onTriggered: root.downloadVisible = root.downloadActive
    }

    function post(message, error) {
        for (let i = 0; i < notices.count; ++i) {
            if (!notices.get(i).closing && notices.get(i).message === message)
                return;
        }
        const notice = { noticeId: nextId++, message: message, error: error, closing: false };
        if (notices.count < 3)
            notices.append(notice);
        else
            pending.push(notice);
    }

    function dismiss(id) {
        for (let i = 0; i < notices.count; ++i) {
            if (notices.get(i).noticeId === id)
                notices.setProperty(i, "closing", true);
        }
    }

    function discard(id) {
        for (let i = 0; i < notices.count; ++i) {
            if (notices.get(i).noticeId === id) {
                notices.remove(i);
                break;
            }
        }
        if (pending.length > 0)
            notices.append(pending.shift());
    }

    spacing: 0

    ListModel {
        id: notices
    }

    Connections {
        target: Toasts

        function onPosted(message, error) {
            root.post(message, error);
        }
    }

    TransferNotice {
        width: root.width
        active: root.downloadVisible
        label: Downloads.batchTotal > 1
            ? qsTr("Downloading %1 / %2").arg(Downloads.batchDone).arg(Downloads.batchTotal)
            : qsTr("Downloading %1").arg(Downloads.activeTitle)
        progress: Downloads.progress
    }

    TransferNotice {
        width: root.width
        active: Uploads.activeName.length > 0
        label: Uploads.batchTotal > 1
            ? qsTr("Uploading %1 / %2").arg(Uploads.batchDone).arg(Uploads.batchTotal)
            : qsTr("Uploading %1").arg(Uploads.activeName)
        progress: Uploads.progress
    }

    Repeater {
        model: notices

        delegate: Item {
            id: notice

            required property int noticeId
            required property string message
            required property bool error
            required property bool closing

            property bool entered: false
            property real remaining
            property real reveal: 0

            readonly property real gap: 10
            readonly property real rise: 28
            readonly property real settled: Math.min(1, notice.reveal)
            readonly property color colAccent: notice.error ? Theme.colError : Theme.colPrimary

            width: root.width
            height: (card.implicitHeight + notice.gap) * notice.settled
            opacity: notice.settled

            states: State {
                name: "shown"
                when: notice.entered && !notice.closing

                PropertyChanges {
                    notice.reveal: 1
                }
            }

            transitions: [
                Transition {
                    to: "shown"
                    NumberAnimation {
                        property: "reveal"
                        duration: Theme.duration.spatialFast
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: Theme.curve.expressiveFastSpatial
                    }
                },
                Transition {
                    from: "shown"
                    SequentialAnimation {
                        NumberAnimation {
                            property: "reveal"
                            duration: Theme.duration.exit
                            easing.type: Easing.BezierSpline
                            easing.bezierCurve: Theme.curve.emphasizedAccel
                        }
                        ScriptAction { script: root.discard(notice.noticeId) }
                    }
                }
            ]

            Item {
                id: plate

                width: parent.width
                height: card.implicitHeight
                y: notice.height - height

                transform: Translate { y: (1 - notice.reveal) * notice.rise }

                RectangularShadow {
                    anchors.fill: card
                    radius: card.radius
                    blur: 28
                    spread: 1
                    offset: Qt.vector2d(0, 4)
                    color: ColorUtils.withAlpha(Theme.colShadow, 0.34)
                }

                Rectangle {
                    id: card

                    anchors.fill: parent
                    implicitHeight: Math.max(60, label.implicitHeight + 38)
                    radius: Theme.rounding.normal
                    color: Theme.colLayer4
                    Accessible.role: Accessible.AlertMessage
                    Accessible.name: notice.message

                    TapHandler {
                        gesturePolicy: TapHandler.ReleaseWithinBounds
                    }

                    Item {
                        id: body

                        anchors.fill: parent
                        anchors.bottomMargin: countdown.height + 6

                        Sym {
                            id: glyph

                            anchors.left: parent.left
                            anchors.leftMargin: 18
                            anchors.verticalCenter: parent.verticalCenter
                            text: notice.error ? "error" : "check_circle"
                            iconSize: 22
                            fill: 1
                            color: notice.colAccent
                        }

                        StyledText {
                            id: label

                            anchors.left: glyph.right
                            anchors.right: dismiss.left
                            anchors.leftMargin: 14
                            anchors.rightMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                            text: notice.message
                            wrapMode: Text.WordWrap
                        }

                        IconButton {
                            id: dismiss

                            anchors.right: parent.right
                            anchors.rightMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                            icon: "close"
                            iconSize: 18
                            diameter: 34
                            Accessible.name: qsTr("Dismiss notification")
                            onClicked: root.dismiss(notice.noticeId)
                        }
                    }

                    Rectangle {
                        id: countdown

                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 7
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: card.radius
                        anchors.rightMargin: card.radius
                        height: 3
                        radius: Theme.rounding.full
                        color: ColorUtils.withAlpha(Theme.colOnSurface, 0.10)

                        Rectangle {
                            width: parent.width * notice.remaining
                            height: parent.height
                            radius: parent.radius
                            color: ColorUtils.withAlpha(notice.colAccent, 0.65)
                        }
                    }

                    HoverHandler {
                        id: hover
                    }
                }
            }

            NumberAnimation on remaining {
                from: 1
                to: 0
                duration: notice.error ? 9000 : 5000
                paused: hover.hovered || !notice.Window.active
                onFinished: root.dismiss(notice.noticeId)
            }

            Component.onCompleted: notice.entered = true
        }
    }
}
