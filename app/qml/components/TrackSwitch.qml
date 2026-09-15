import QtQuick
import HtMusic

Item {
    id: root

    property real travel: 48
    property real dip: 0.06
    property real clear: 1.1
    property bool vertical: false
    property var frozen
    property real phase: 0
    property int heading: 1
    property bool holding: false

    readonly property var live: root.capture()
    readonly property var track: root.holding && root.frozen ? root.frozen : root.live
    readonly property real offsetX: root.vertical ? 0 : -root.phase * root.travel
    readonly property real offsetY: root.vertical ? -root.phase * root.travel : 0
    readonly property real fade: Math.max(0, 1 - Math.abs(root.phase) * root.clear)
    readonly property real depth: 1 - Math.abs(root.phase) * root.dip

    function capture() {
        const entry = PlaybackController.track;
        return {
            valid: entry.valid,
            title: entry.title,
            artist: entry.artist,
            album: entry.album,
            artId: entry.artId,
            video: entry.video,
            live: entry.live,
            episode: entry.episode
        };
    }

    Connections {
        target: PlaybackController

        function onTrackAdvanced(direction) {
            root.heading = direction;
            root.holding = true;
            swap.restart();
        }
    }

    SequentialAnimation {
        id: swap

        NumberAnimation {
            target: root
            property: "phase"
            to: root.heading
            duration: Theme.duration.exit
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.emphasizedAccel
        }

        ScriptAction {
            script: {
                root.holding = false;
                root.phase = -root.heading;
            }
        }

        NumberAnimation {
            target: root
            property: "phase"
            to: 0
            duration: Theme.duration.enter
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.emphasizedDecel
        }
    }

    onLiveChanged: if (!root.holding) root.frozen = root.live

    Component.onCompleted: root.frozen = root.live
}
