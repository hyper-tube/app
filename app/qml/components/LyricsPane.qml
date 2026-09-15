import QtQuick
import QtQuick.Controls
import HtMusic

Item {
    id: root

    readonly property real emphasisScale: 1.16
    readonly property real scaleReserve: Lyrics.synced ? 1.2 : 1
    readonly property real rightInset: 24
    readonly property bool populated: Lyrics.available

    function follow() {
        if (!Lyrics.synced || Lyrics.activeLine < 0 || !root.visible)
            return;

        const origin = lines.contentY;
        glide.stop();
        lines.positionViewAtIndex(Lyrics.activeLine, ListView.Center);
        const target = lines.contentY;
        if (target === origin)
            return;

        lines.contentY = origin;
        glide.from = origin;
        glide.to = target;
        glide.start();
    }

    ErrorState {
        anchors.fill: parent
        size: ErrorState.Pane
        shown: Lyrics.interrupted && !root.populated
        busy: Connectivity.checking
        icon: "cloud_off"
        title: qsTr("Lyrics need a connection")
        caption: qsTr("They show up here as soon as you're back online.")
        onActionTriggered: Lyrics.retry()
    }

    EmptyState {
        anchors.fill: parent
        visible: !root.populated && !Lyrics.loading && !Lyrics.interrupted
        icon: "lyrics"
        title: qsTr("No lyrics")
        caption: PlaybackController.track.valid
            ? qsTr("Lyrics are not available for this track")
            : qsTr("Play a track to see its lyrics")
    }

    BusySpinner {
        anchors.centerIn: parent
        visible: Lyrics.loading && !root.populated
    }

    ListView {
        id: lines

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: source.top
        anchors.bottomMargin: 8
        visible: root.populated
        model: Lyrics.lines
        spacing: 10
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        topMargin: 12
        bottomMargin: 24

        ScrollBar.vertical: PageScrollBar {}

        delegate: Item {
            id: line

            required property string modelData
            required property int index

            readonly property bool spoken: Lyrics.synced && line.index === Lyrics.activeLine
            readonly property bool passed: Lyrics.synced && line.index < Lyrics.activeLine

            width: lines.width
            height: label.implicitHeight * root.scaleReserve

            StyledText {
                id: label

                width: Math.max(1, (line.width - root.rightInset) / root.scaleReserve)
                renderType: Text.CurveRendering
                text: line.modelData
                title: line.spoken
                wrapMode: Text.Wrap
                transformOrigin: Item.Left
                scale: line.spoken ? root.emphasisScale : 1
                font.pixelSize: Lyrics.synced ? Theme.font.larger : Theme.font.normal
                color: !Lyrics.synced ? Theme.colOnSurface
                    : line.spoken ? Theme.colPrimary
                    : line.passed ? Theme.colOnSurfaceVariant
                    : Theme.colInactive

                Behavior on scale {
                    NumberAnimation {
                        duration: Theme.duration.spatialFast
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: Theme.curve.expressiveFastSpatial
                    }
                }

                Behavior on color {
                    ColorAnimation {
                        duration: Theme.duration.fast
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: Theme.curve.expressiveEffects
                    }
                }
            }
        }

        ScrollWheel {
            target: lines
        }
    }

    Item {
        id: source

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: attribution.visible ? attribution.contentHeight + 6 : 0

        StyledText {
            id: attribution

            width: parent.width
            visible: root.populated && Lyrics.source.length > 0
            text: Lyrics.source
            font.pixelSize: Theme.font.smallest
            color: Theme.colInactive
            elide: Text.ElideRight
        }
    }

    NumberAnimation {
        id: glide

        target: lines
        property: "contentY"
        duration: Theme.duration.spatialFast
        easing.type: Easing.BezierSpline
        easing.bezierCurve: Theme.curve.emphasized

        onFinished: lines.returnToBounds()
    }

    Connections {
        target: Lyrics

        function onActiveLineChanged() {
            root.follow();
        }

        function onChanged() {
            glide.stop();
            if (root.populated)
                lines.positionViewAtBeginning();
        }
    }
}
