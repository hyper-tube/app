import QtQuick
import HtMusic

Item {
    id: root

    required property var lead
    required property var cells
    required property ItemModel entries

    property real bloom: 0

    readonly property bool listed: root.cells.length > 0
    readonly property bool wide: root.width >= 1000
    readonly property bool collection: root.lead.kind === "album" || root.lead.kind === "playlist"
        || root.lead.kind === "podcast"
    readonly property bool person: root.lead.circular
    readonly property bool shuffles: root.lead.actions.shufflePlaylistId.length > 0
    readonly property bool writable: Account.signedIn && Connectivity.online
    readonly property real plateHeight: 196
    readonly property real rowHeight: 58
    readonly property real listHeight: root.cells.length * root.rowHeight
    readonly property bool active: root.lead.playable
        && PlaybackController.track.videoId === root.lead.track.videoId

    signal menuRequested(var request)

    implicitHeight: root.wide || !root.listed ? root.plateHeight : root.plateHeight + 12 + root.listHeight

    function requestMenu(source, x, y) {
        root.menuRequested({ "entry": root.lead, "index": -1, "model": null, "source": source,
                             "x": x, "y": y, "selectable": false });
    }

    RippleSurface {
        id: plate

        width: root.wide && root.listed ? Math.round(root.width * 0.48) : root.width
        height: root.plateHeight
        rounding: Theme.rounding.large
        colBackground: Theme.colLayer2
        Accessible.role: Accessible.Button
        Accessible.name: root.lead.title

        Item {
            anchors.fill: parent
            layer.enabled: true
            layer.effect: RoundedMask {
                rounding: plate.rounding
            }

            AmbientArt {
                anchors.fill: parent
                artId: root.lead.artId
                opacity: 0.55 * root.bloom
            }

            Rectangle {
                anchors.fill: parent
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: ColorUtils.withAlpha(Theme.colLayer2, 0.35) }
                    GradientStop { position: 1.0; color: ColorUtils.withAlpha(Theme.colLayer2, 0.92) }
                }
            }
        }

        Item {
            id: cover

            x: 20
            anchors.verticalCenter: parent.verticalCenter
            width: 156
            height: 156
            scale: 0.88 + 0.12 * root.bloom

            Artwork {
                anchors.fill: parent
                artId: root.lead.artId
                rounding: root.person ? Theme.rounding.full : Theme.rounding.normal
            }

            LiveBadge {
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.margins: 8
                width: implicitWidth
                height: implicitHeight
                visible: root.lead.track.live
                pulsing: root.active && PlaybackController.playing
            }

            Rectangle {
                anchors.centerIn: parent
                width: 54
                height: 54
                radius: Theme.rounding.full
                color: ColorUtils.withAlpha(Theme.colLayer1, 0.85)
                visible: root.active

                PlayingIndicator {
                    anchors.centerIn: parent
                    barWidth: 4
                    barHeight: 24
                    barSpacing: 4
                    playing: root.active && PlaybackController.playing
                }
            }
        }

        Column {
            anchors.left: cover.right
            anchors.leftMargin: 22
            anchors.right: parent.right
            anchors.rightMargin: 18
            anchors.verticalCenter: parent.verticalCenter
            spacing: 4
            opacity: Math.min(1, root.bloom * 1.4)
            transform: Translate { x: 16 * (1 - root.bloom) }

            StyledText {
                width: parent.width
                text: qsTr("Top result")
                title: true
                font.pixelSize: Theme.font.smallest
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 0.8
                color: Theme.colPrimary
                elide: Text.ElideRight
            }

            StyledText {
                width: parent.width
                text: root.lead.title
                title: true
                font.pixelSize: Theme.font.huge
                color: root.active ? Theme.colPrimary : Theme.colOnSurface
                wrapMode: Text.WordWrap
                maximumLineCount: 2
                elide: Text.ElideRight
            }

            StyledText {
                width: parent.width
                visible: root.lead.subtitle.length > 0
                text: root.lead.subtitle
                font.pixelSize: Theme.font.smallie
                color: Theme.colOnSurfaceVariant
                elide: Text.ElideRight
            }

            Item {
                width: 1
                height: 8
            }

            Row {
                spacing: 8

                PillButton {
                    visible: root.lead.playable || root.collection || root.shuffles
                    text: root.lead.playable || root.collection ? qsTr("Play") : qsTr("Shuffle")
                    icon: root.lead.playable || root.collection ? "play_arrow" : "shuffle"
                    toggled: true
                    interactive: Connectivity.online || root.lead.playable
                    onClicked: {
                        if (root.lead.playable || root.collection)
                            Browser.playEntry(root.lead);
                        else
                            Browser.playShuffled(root.lead);
                    }
                }

                PillButton {
                    visible: root.collection && Connectivity.online
                    text: qsTr("Shuffle")
                    icon: "shuffle"
                    onClicked: Browser.playShuffled(root.lead)
                }

                PillButton {
                    visible: root.person && root.lead.actions.mixable && Connectivity.online
                    text: qsTr("Mix")
                    icon: "radio"
                    onClicked: Browser.playMix(root.lead)
                }

                PillButton {
                    visible: root.lead.playable && root.writable
                    text: qsTr("Save")
                    icon: "playlist_add"
                    onClicked: PlaylistTargets.show([root.lead.track.videoId])
                }

                IconButton {
                    id: overflow

                    anchors.verticalCenter: parent.verticalCenter
                    icon: "more_vert"
                    Accessible.name: qsTr("More actions")
                    onClicked: {
                        const spot = plate.mapFromItem(overflow, overflow.width / 2, overflow.height);
                        root.requestMenu(plate, spot.x, spot.y);
                    }
                }
            }
        }

        onClicked: Browser.open(root.lead)
        onRightClicked: (x, y) => root.requestMenu(plate, x, y)
    }

    Column {
        x: root.wide ? plate.width + 16 : 0
        y: root.wide ? Math.max(0, (root.plateHeight - root.listHeight) / 2) : root.plateHeight + 12
        width: root.wide ? root.width - x : root.width
        visible: root.listed

        Repeater {
            model: root.cells

            delegate: SongRow {
                id: child

                required property var modelData
                required property int index

                readonly property real arrival: Math.max(0, Math.min(1, root.bloom * 1.6 - child.index * 0.18))

                width: parent.width
                implicitHeight: root.rowHeight
                numbered: false
                showAlbum: false
                title: child.modelData.title
                artist: child.modelData.track.artist || child.modelData.subtitle
                artId: child.modelData.artId
                durationMs: child.modelData.track.durationMs
                live: child.modelData.track.live
                videoId: child.modelData.track.videoId
                liked: child.modelData.track.liked
                downloaded: child.modelData.playable && Downloads.ids.includes(child.modelData.track.videoId)
                playable: child.modelData.playable
                circular: child.modelData.circular
                active: child.modelData.playable
                    && PlaybackController.track.videoId === child.modelData.track.videoId
                opacity: child.arrival
                transform: Translate { x: 20 * (1 - child.arrival) }
                onPlayRequested: Browser.activate(root.entries, child.index)
                onMenuRequested: (x, y) => root.menuRequested({
                    "entry": child.modelData, "index": child.index, "model": root.entries,
                    "source": child, "x": x, "y": y, "selectable": false })
            }
        }
    }

    NumberAnimation {
        id: arrive

        target: root
        property: "bloom"
        from: 0
        to: 1
        duration: Theme.duration.spatialSlow
        easing.type: Easing.BezierSpline
        easing.bezierCurve: Theme.curve.expressiveDefaultSpatial
    }

    Component.onCompleted: arrive.start()
}
