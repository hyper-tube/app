import QtQuick
import HtMusic

RippleSurface {
    id: root

    property int position: 0
    property bool numbered: true
    property bool circular: false
    property string title: ""
    property string artist: ""
    property string album: ""
    property string artId: ""
    property int durationMs: 0
    property bool live: false
    property bool active: false
    property bool showAlbum: true
    property bool playable: true
    property string videoId: ""
    property bool liked: false
    property bool downloaded: false
    property bool reorderable: false
    property bool reorderEnabled: true
    property bool selectMode: false
    property bool selected: false
    property real selectReveal: root.selectMode ? 1 : 0
    property real reorderReveal: root.reorderable ? 1 : 0
    property real reorderReadiness: root.reorderEnabled ? 1 : 0
    property real presence: root.reachable ? 1 : 0.55

    readonly property bool reachable: Connectivity.online || root.downloaded || !root.playable
    readonly property bool playingNow: root.active && PlaybackController.playing
    readonly property bool cueVisible: root.hovered || root.active
    readonly property real checkInset: 32 * root.selectReveal

    signal playRequested
    signal menuRequested(real x, real y)
    signal dragStarted(real x, real y)
    signal dragMoved(real x, real y)
    signal dragFinished(bool cancelled)
    signal selectToggled

    implicitHeight: 60
    rounding: Theme.rounding.small
    colState: Theme.colOnSurface

    Behavior on selectReveal {
        NumberAnimation {
            duration: Theme.duration.spatialFast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveFastSpatial
        }
    }

    Behavior on reorderReveal {
        NumberAnimation {
            duration: Theme.duration.spatialFast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveFastSpatial
        }
    }

    Behavior on reorderReadiness {
        NumberAnimation {
            duration: Theme.duration.fast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveEffects
        }
    }

    Behavior on presence {
        NumberAnimation {
            duration: Theme.duration.fast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveEffects
        }
    }

    CheckMark {
        anchors.left: parent.left
        anchors.leftMargin: 14
        anchors.verticalCenter: parent.verticalCenter
        checked: root.selected
        visible: root.selectReveal > 0
        opacity: root.selectReveal
        scale: 0.6 + 0.4 * root.selectReveal
    }

    StyledText {
        id: ordinal

        anchors.left: parent.left
        anchors.leftMargin: 14 + root.checkInset
        anchors.verticalCenter: parent.verticalCenter
        width: root.numbered ? 24 : 0
        visible: root.numbered
        opacity: root.cueVisible ? 0 : 1
        text: root.position
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: Theme.font.smaller
        color: Theme.colInactive

        Behavior on opacity {
            NumberAnimation {
                duration: Theme.duration.fast
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveEffects
            }
        }
    }

    Sym {
        anchors.horizontalCenter: ordinal.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        visible: root.numbered
        opacity: root.cueVisible && !root.playingNow ? 1 : 0
        text: root.playable ? "play_arrow" : "chevron_right"
        iconSize: Theme.font.larger
        fill: 1
        color: root.active ? Theme.colPrimary : Theme.colOnSurface

        Behavior on opacity {
            NumberAnimation {
                duration: Theme.duration.fast
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveEffects
            }
        }
    }

    PlayingIndicator {
        anchors.horizontalCenter: ordinal.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        width: 24
        visible: root.numbered && root.playingNow
        playing: visible && enabled
    }

    Artwork {
        id: cover

        anchors.left: root.numbered ? ordinal.right : parent.left
        anchors.leftMargin: root.numbered ? 14 : 14 + root.checkInset
        anchors.verticalCenter: parent.verticalCenter
        width: 44
        height: 44
        opacity: root.presence
        artId: root.artId
        rounding: root.circular ? Theme.rounding.full : Theme.rounding.verysmall

        Rectangle {
            id: cue

            anchors.fill: parent
            radius: cover.rounding
            color: ColorUtils.withAlpha(Theme.colLayer1, 0.62)
            visible: !root.numbered
            opacity: root.cueVisible ? 1 : 0

            Behavior on opacity {
                NumberAnimation {
                    duration: Theme.duration.fast
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Theme.curve.expressiveEffects
                }
            }

            Sym {
                anchors.centerIn: parent
                opacity: root.playingNow ? 0 : 1
                text: root.playable ? "play_arrow" : "chevron_right"
                iconSize: Theme.font.larger
                fill: 1
                color: Theme.colOnSurface
            }

            PlayingIndicator {
                anchors.centerIn: parent
                visible: root.playingNow
                playing: visible && enabled
            }
        }
    }

    Column {
        id: names

        anchors.left: cover.right
        anchors.leftMargin: 14
        anchors.right: albumLabel.left
        anchors.rightMargin: 18
        anchors.verticalCenter: parent.verticalCenter
        opacity: root.presence
        spacing: 2

        StyledText {
            width: parent.width
            text: root.title
            title: true
            font.pixelSize: Theme.font.small
            color: root.active ? Theme.colPrimary : Theme.colOnSurface
            elide: Text.ElideRight
        }

        StyledText {
            width: parent.width
            text: root.artist
            font.pixelSize: Theme.font.smaller
            color: Theme.colInactive
            elide: Text.ElideRight
        }
    }

    StyledText {
        id: albumLabel

        anchors.right: trailing.left
        anchors.rightMargin: 18
        anchors.verticalCenter: parent.verticalCenter
        width: root.showAlbum ? Math.max(0, root.width * 0.24) : 0
        visible: root.showAlbum && root.width > 720
        opacity: root.presence
        text: root.album
        font.pixelSize: Theme.font.smaller
        color: Theme.colInactive
        elide: Text.ElideRight
    }

    Row {
        id: trailing

        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        spacing: 4

        Sym {
            anchors.verticalCenter: parent.verticalCenter
            opacity: root.downloaded ? 1 : 0
            text: "download_done"
            iconSize: Theme.font.normal
            color: Theme.colPrimary
            Accessible.ignored: !root.downloaded
            Accessible.name: qsTr("Downloaded")

            Behavior on opacity {
                NumberAnimation {
                    duration: Theme.duration.fast
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Theme.curve.expressiveEffects
                }
            }
        }

        Item {
            anchors.verticalCenter: parent.verticalCenter
            width: Math.max(44, root.live ? liveBadge.implicitWidth : 0)
            height: liveBadge.implicitHeight

            StyledText {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                visible: !root.live
                text: root.playable && root.durationMs > 0
                    ? PlaybackController.formatTime(root.durationMs) : ""
                horizontalAlignment: Text.AlignRight
                font.pixelSize: Theme.font.smaller
                color: Theme.colInactive
            }

            LiveBadge {
                id: liveBadge

                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: implicitWidth
                height: implicitHeight
                visible: root.live
                filled: false
                pulsing: root.active
            }
        }

        LikeButton {
            anchors.verticalCenter: parent.verticalCenter
            visible: Account.signedIn && root.videoId.length > 0
            opacity: root.liked || root.hovered ? 1 : 0
            iconSize: Theme.font.normal
            diameter: 36
            videoId: root.videoId
            liked: root.liked
            interactive: canRate && opacity > 0

            Behavior on opacity {
                NumberAnimation {
                    duration: Theme.duration.fast
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Theme.curve.expressiveEffects
                }
            }
        }

        Item {
            id: handle

            anchors.verticalCenter: parent.verticalCenter
            width: 32 * root.reorderReveal
            height: 36
            visible: root.reorderReveal > 0
            opacity: root.reorderReveal * (0.4 + 0.6 * root.reorderReadiness)
            enabled: root.reorderEnabled
            Accessible.role: Accessible.Grip
            Accessible.name: qsTr("Drag to reorder %1").arg(root.title)

            Sym {
                anchors.centerIn: parent
                text: "drag_handle"
                iconSize: 20
                color: hover.hovered || drag.active ? Theme.colPrimary : Theme.colInactive
            }

            HoverHandler {
                id: hover
                cursorShape: drag.active ? Qt.ClosedHandCursor : Qt.OpenHandCursor
            }

            TapHandler {
                gesturePolicy: TapHandler.ReleaseWithinBounds
            }

            DragHandler {
                id: drag

                target: null
                acceptedButtons: Qt.LeftButton
                xAxis.enabled: false
                onActiveChanged: {
                    if (active) {
                        const point = handle.mapToItem(root, centroid.position.x, centroid.position.y);
                        root.dragStarted(point.x, point.y);
                    } else {
                        root.dragFinished(false);
                    }
                }
                onCentroidChanged: {
                    if (!active)
                        return;
                    const point = handle.mapToItem(root, centroid.position.x, centroid.position.y);
                    root.dragMoved(point.x, point.y);
                }
                onCanceled: root.dragFinished(true)
            }
        }

        IconButton {
            id: overflow

            anchors.verticalCenter: parent.verticalCenter
            icon: "more_vert"
            iconSize: Theme.font.normal
            diameter: 36
            opacity: root.hovered ? 1 : 0
            interactive: root.hovered
            Accessible.name: qsTr("More actions")
            onClicked: {
                const spot = root.mapFromItem(overflow, overflow.width / 2, overflow.height);
                root.menuRequested(spot.x, spot.y);
            }

            Behavior on opacity {
                NumberAnimation {
                    duration: Theme.duration.fast
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Theme.curve.expressiveEffects
                }
            }
        }
    }

    onClicked: {
        if (root.selectMode)
            root.selectToggled();
        else
            root.playRequested();
    }
    onRightClicked: (x, y) => root.menuRequested(x, y)
}
