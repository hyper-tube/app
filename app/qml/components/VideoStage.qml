import QtQuick
import HtMusic

Item {
    id: root

    property string artId: ""
    property bool video: false
    property bool live: false
    property real reach: 100
    property real span: 400

    readonly property bool offered: PlaybackController.videoAvailable
    readonly property bool watching: root.offered && PlaybackController.videoShown
    readonly property real shape: root.watching && PlaybackController.videoLive
        ? PlaybackController.videoAspect
        : root.video || root.watching ? 16 / 9 : 1

    property real aspect: root.shape
    property real reveal: root.watching && PlaybackController.videoLive ? 1 : 0

    width: Math.min(root.span, root.reach * root.aspect)
    height: root.width / root.aspect
    scale: PlaybackController.playing ? 1 : 0.94

    Behavior on aspect {
        NumberAnimation {
            duration: Theme.duration.enter
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.emphasizedDecel
        }
    }

    Behavior on reveal {
        NumberAnimation {
            duration: Theme.duration.spatial
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.emphasizedDecel
        }
    }

    Behavior on scale {
        NumberAnimation {
            duration: Theme.duration.spatialSlow
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveSlowSpatial
        }
    }

    Artwork {
        anchors.fill: parent
        artId: root.artId
        rounding: Theme.rounding.large
    }

    VideoSurface {
        anchors.fill: parent
        visible: root.watching || root.reveal > 0
        opacity: root.reveal
        scale: 0.97 + 0.03 * root.reveal
        layer.enabled: visible
        layer.effect: RoundedMask {
            rounding: Theme.rounding.large
        }
    }

    BusySpinner {
        anchors.centerIn: parent
        opacity: root.watching && !PlaybackController.videoLive ? 1 : 0

        Behavior on opacity {
            NumberAnimation {
                duration: Theme.duration.fast
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveEffects
            }
        }
    }

    LiveBadge {
        property real appear: root.live ? 1 : 0

        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 14
        width: implicitWidth
        height: implicitHeight
        pixelSize: Theme.font.smaller
        pulsing: PlaybackController.playing
        opacity: appear
        scale: 0.82 + 0.18 * appear
        visible: appear > 0
        transformOrigin: Item.TopLeft

        Behavior on appear {
            NumberAnimation {
                duration: Theme.duration.spatialFast
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveFastSpatial
            }
        }
    }

    VideoModeSwitch {
        property real appear: root.offered ? 1 : 0

        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 12
        video: PlaybackController.videoShown
        opacity: appear
        scale: 0.82 + 0.18 * appear
        visible: appear > 0
        transformOrigin: Item.BottomRight
        onPicked: wantsVideo => PlaybackController.videoShown = wantsVideo

        Behavior on appear {
            NumberAnimation {
                duration: Theme.duration.spatialFast
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveFastSpatial
            }
        }
    }
}
