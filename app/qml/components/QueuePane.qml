import QtQuick
import HtMusic

Item {
    id: root

    property real originReveal: root.framed ? 1 : 0

    readonly property var origin: PlaybackController.queueSource
    readonly property bool navigable: root.origin.valid
    readonly property bool station: PlaybackController.live
    readonly property bool framed: root.navigable || root.station
    readonly property real originPresence: Math.max(0, Math.min(1, root.originReveal))

    signal sourceRequested(var source)

    function originLabel() {
        if (!root.navigable)
            return qsTr("Live station");
        switch (root.origin.kind) {
        case "album":
            return qsTr("Playing from album");
        case "artist":
            return qsTr("Playing from artist");
        case "library":
            return qsTr("Playing from your library");
        case "mix":
            return qsTr("Playing from mix");
        case "podcast":
            return qsTr("Playing from podcast");
        default:
            return qsTr("Playing from playlist");
        }
    }

    function openMenu(source, position, x, y) {
        if (position === PlaybackController.queueIndex)
            return;
        menu.target = position;
        menu.show({ "entry": PlaybackController.queueEntry(position), "index": position,
                    "model": null, "source": source, "x": x, "y": y, "selectable": false });
    }

    function centerCurrent() {
        if (list.currentIndex < 0 || !root.visible)
            return;
        glide.stop();
        list.positionViewAtIndex(list.currentIndex, ListView.Center);
    }

    function glideToCurrent() {
        if (list.currentIndex < 0 || !root.visible)
            return;
        const origin = list.contentY;
        glide.stop();
        list.positionViewAtIndex(list.currentIndex, ListView.Center);
        const target = list.contentY;
        if (target === origin)
            return;
        list.contentY = origin;
        glide.from = origin;
        glide.to = target;
        glide.start();
    }

    Behavior on originReveal {
        NumberAnimation {
            duration: Theme.duration.spatial
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.emphasized
        }
    }

    Item {
        id: originFrame

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: (originPlate.implicitHeight + 10) * root.originPresence
        visible: height > 0
        clip: true

        RippleSurface {
            id: originPlate

            width: parent.width
            implicitHeight: 64
            y: -(1 - root.originPresence) * 16
            opacity: root.originPresence
            rounding: Theme.rounding.normal
            colBackground: ColorUtils.withAlpha(Theme.colLayer2, 0.6)
            interactive: root.navigable
            Accessible.role: root.navigable ? Accessible.Button : Accessible.StaticText
            Accessible.name: root.originLabel() + ", " + originTitle.text

            Artwork {
                id: originArt

                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                width: 44
                height: 44
                artId: root.navigable && root.origin.artId.length > 0 ? root.origin.artId
                    : PlaybackController.track.artId
                rounding: root.navigable && root.origin.circular ? Theme.rounding.full
                    : Theme.rounding.verysmall
            }

            Column {
                anchors.left: originArt.right
                anchors.leftMargin: 12
                anchors.right: originCue.left
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2

                StyledText {
                    width: parent.width
                    text: root.originLabel()
                    title: true
                    font.pixelSize: Theme.font.smallest
                    font.capitalization: Font.AllUppercase
                    font.letterSpacing: 0.6
                    color: root.navigable ? Theme.colPrimary : Theme.colError
                    elide: Text.ElideRight
                }

                StyledText {
                    id: originTitle

                    width: parent.width
                    text: root.navigable ? root.origin.title : PlaybackController.track.title
                    title: true
                    font.pixelSize: Theme.font.smallie
                    elide: Text.ElideRight
                }
            }

            Item {
                id: originCue

                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                width: root.navigable ? chevron.iconSize : stationBadge.implicitWidth
                height: stationBadge.implicitHeight

                Sym {
                    id: chevron

                    anchors.centerIn: parent
                    visible: root.navigable
                    text: "chevron_right"
                    iconSize: Theme.font.larger
                    color: originPlate.hovered ? Theme.colOnSurface : Theme.colOnSurfaceVariant
                }

                LiveBadge {
                    id: stationBadge

                    anchors.centerIn: parent
                    width: implicitWidth
                    height: implicitHeight
                    visible: !root.navigable
                    filled: false
                    pulsing: PlaybackController.playing
                }
            }

            onClicked: root.sourceRequested(root.origin)
        }
    }

    ListView {
        id: list

        anchors.top: originFrame.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        spacing: 2
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        acceptedButtons: Qt.NoButton
        reuseItems: true
        cacheBuffer: 116
        model: PlaybackController.queueWindow
        currentIndex: PlaybackController.queueIndex - PlaybackController.queueOffset

        delegate: RippleSurface {
            id: row

            required property var modelData
            required property int index

            property real presence: row.reachable ? 1 : 0.55

            readonly property int queuePosition: row.index + PlaybackController.queueOffset
            readonly property bool current: row.queuePosition === PlaybackController.queueIndex
            readonly property bool live: row.current ? PlaybackController.live : row.modelData.live
            readonly property bool reachable: Connectivity.online
                || Downloads.ids.indexOf(row.modelData.videoId) >= 0

            width: list.width
            implicitHeight: 56
            rounding: Theme.rounding.small

            Behavior on presence {
                NumberAnimation {
                    duration: Theme.duration.fast
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Theme.curve.expressiveEffects
                }
            }

            Artwork {
                id: thumb

                anchors.left: parent.left
                anchors.leftMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                width: 40
                height: 40
                opacity: row.presence
                artId: modelData.artId
                rounding: Theme.rounding.verysmall
            }

            Column {
                anchors.left: thumb.right
                anchors.leftMargin: 12
                anchors.right: length.left
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                opacity: row.presence
                spacing: 1

                StyledText {
                    width: parent.width
                    text: modelData.title
                    title: row.queuePosition === PlaybackController.queueIndex
                    font.pixelSize: Theme.font.smallie
                    color: row.queuePosition === PlaybackController.queueIndex
                        ? Theme.colPrimary
                        : Theme.colOnSurface
                    elide: Text.ElideRight
                }

                StyledText {
                    width: parent.width
                    text: modelData.artist
                    font.pixelSize: Theme.font.smaller
                    color: Theme.colInactive
                    elide: Text.ElideRight
                }
            }

            LiveBadge {
                id: rowLive

                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                width: implicitWidth
                height: implicitHeight
                visible: row.live
                filled: false
                pulsing: row.current && PlaybackController.playing
            }

            StyledText {
                id: length

                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                opacity: row.reachable && !row.live ? 1 : 0
                width: row.live ? rowLive.implicitWidth : implicitWidth
                text: PlaybackController.formatTime(modelData.durationMs)
                font.pixelSize: Theme.font.smaller
                color: Theme.colInactive
            }

            Sym {
                anchors.right: parent.right
                anchors.rightMargin: 14
                anchors.verticalCenter: parent.verticalCenter
                opacity: 1 - row.presence
                text: "cloud_off"
                iconSize: Theme.font.normal
                color: Theme.colInactive
                Accessible.ignored: row.reachable
                Accessible.name: qsTr("Not downloaded")
            }

            onClicked: PlaybackController.playAt(row.queuePosition)
            onRightClicked: (x, y) => root.openMenu(row, row.queuePosition, x, y)
        }
    }

    Item {
        anchors.fill: list

        ScrollWheel {
            target: list
        }
    }

    ItemMenu {
        id: menu

        property int target: -1

        parent: root
        extraActions: [{ "icon": "playlist_remove", "text": qsTr("Remove from queue"),
                         "action": "dequeue", "destructive": true }]
        onExtraTriggered: PlaybackController.removeFromQueue(menu.target)
    }

    NumberAnimation {
        id: glide

        target: list
        property: "contentY"
        duration: Theme.duration.spatial
        easing.type: Easing.BezierSpline
        easing.bezierCurve: Theme.curve.emphasized

        onFinished: list.returnToBounds()
    }

    Connections {
        target: PlaybackController

        function onQueueChanged() {
            Qt.callLater(root.centerCurrent);
        }

        function onQueueIndexChanged() {
            Qt.callLater(root.glideToCurrent);
        }
    }

    onVisibleChanged: {
        if (visible)
            Qt.callLater(root.centerCurrent);
    }

    Component.onCompleted: Qt.callLater(root.centerCurrent)

    EmptyState {
        anchors.fill: parent
        visible: list.count === 0
        icon: "queue_music"
        title: qsTr("Nothing queued")
        caption: qsTr("Play something to build a queue")
    }
}
