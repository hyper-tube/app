import QtQuick
import QtQuick.Effects
import HtMusic

Item {
    id: root

    required property ListView view
    required property BrowseModel page
    required property real contentWidth

    property ItemModel entries: null
    property contentItem entry
    property int sourceIndex: -1
    property int targetIndex: -1
    property real firstY: 0
    property real stride: 62
    property real pointerX: 0
    property real pointerY: 0
    property real grabOffset: 0
    property real cardY: 0
    property real lift: 0
    property bool dragging: false
    property bool settling: false
    property bool suppressMoves: false
    property bool committing: false

    readonly property bool active: root.dragging || root.settling
    readonly property real targetY: root.firstY + root.targetIndex * root.stride - root.view.contentY

    z: 10

    function begin(delegate, row, x, y) {
        if (root.active || !root.page || !root.page.reorderable || root.page.reordering)
            return;
        root.entries = delegate.entries;
        root.entry = delegate.entry;
        root.sourceIndex = delegate.itemIndex;
        root.targetIndex = root.sourceIndex;
        root.stride = delegate.height + root.view.spacing;
        root.firstY = delegate.y - root.sourceIndex * root.stride;
        const point = row.mapToItem(root, x, y);
        root.pointerX = point.x;
        root.pointerY = point.y;
        root.cardY = delegate.y - root.view.contentY;
        root.grabOffset = point.y - root.cardY;
        root.dragging = true;
        root.forceActiveFocus();
    }

    function update(row, x, y) {
        if (!root.dragging)
            return;
        const point = row.mapToItem(root, x, y);
        root.pointerX = point.x;
        root.pointerY = point.y;
        root.updateTarget();
    }

    function updateTarget() {
        root.cardY = Math.max(0, Math.min(root.height - card.height, root.pointerY - root.grabOffset));
        root.targetIndex = Math.max(0, Math.min(root.entries.count - 1,
            Math.round((root.cardY + root.view.contentY - root.firstY) / root.stride)));
    }

    function offset(index) {
        if (!root.active || index === root.sourceIndex)
            return 0;
        if (index > root.sourceIndex && index <= root.targetIndex)
            return -root.stride;
        if (index < root.sourceIndex && index >= root.targetIndex)
            return root.stride;
        return 0;
    }

    function finish(cancelled) {
        if (!root.dragging)
            return;
        if (cancelled || root.pointerX < 0 || root.pointerX > root.width
            || root.pointerY < 0 || root.pointerY > root.height)
            root.targetIndex = root.sourceIndex;
        root.dragging = false;
        root.settling = true;
        settle.to = root.targetY;
        settle.start();
    }

    function cancel() {
        if (root.committing)
            return;
        settle.stop();
        root.dragging = false;
        root.settling = false;
        root.entries = null;
        root.sourceIndex = -1;
        root.targetIndex = -1;
    }

    Keys.onEscapePressed: event => {
        root.finish(true);
        event.accepted = true;
    }

    states: State {
        name: "lifted"
        when: root.dragging
        PropertyChanges { target: root; lift: 1 }
    }

    transitions: Transition {
        NumberAnimation {
            property: "lift"
            duration: Theme.duration.spatialFast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveFastSpatial
        }
    }

    Rectangle {
        x: card.x
        y: root.targetY
        width: card.width
        height: card.height
        radius: Theme.rounding.small
        visible: root.active
        color: ColorUtils.withAlpha(Theme.colPrimary, 0.06)
        border.width: 1
        border.color: ColorUtils.withAlpha(Theme.colPrimary, 0.25)

        Behavior on y {
            enabled: root.dragging && !scroll.running
            NumberAnimation {
                duration: Theme.duration.spatialFast
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveFastSpatial
            }
        }
    }

    Item {
        id: card

        x: (root.width - root.contentWidth) / 2 + Theme.size.bleed
        y: root.cardY
        width: root.contentWidth - Theme.size.bleed * 2
        height: 60
        visible: root.active
        scale: 1 + 0.012 * root.lift
        enabled: false

        RectangularShadow {
            anchors.fill: parent
            radius: Theme.rounding.small
            blur: 24 * root.lift
            offset: Qt.vector2d(0, 6 * root.lift)
            color: ColorUtils.withAlpha(Theme.colShadow, 0.28 * root.lift)
        }

        SongRow {
            anchors.fill: parent
            position: root.targetIndex + 1
            title: root.entry.title
            artist: root.entry.track.artist || root.entry.subtitle
            album: root.entry.track.album
            artId: root.entry.artId
            videoId: root.entry.track.videoId
            durationMs: root.entry.track.durationMs
            liked: root.entry.track.liked
            downloaded: Downloads.ids.includes(root.entry.track.videoId)
            reorderable: true
            colBackground: Theme.colLayer3
            interactive: false
        }
    }

    NumberAnimation {
        id: settle

        target: root
        property: "cardY"
        duration: Theme.duration.spatialFast
        easing.type: Easing.BezierSpline
        easing.bezierCurve: Theme.curve.expressiveFastSpatial
        onFinished: {
            const moved = root.sourceIndex !== root.targetIndex;
            const destination = root.targetIndex;
            const id = root.entry.actions.setVideoId;
            root.committing = true;
            root.suppressMoves = true;
            root.settling = false;
            if (moved && root.page)
                root.page.movePlaylistItem(id, destination);
            root.view.forceLayout();
            root.committing = false;
            root.cancel();
            Qt.callLater(() => root.suppressMoves = false);
        }
    }

    Timer {
        id: scroll

        interval: 16
        repeat: true
        running: root.dragging && (root.pointerY < 64 || root.pointerY > root.height - 64)
        onTriggered: {
            const edge = root.pointerY < 64 ? root.pointerY - 64 : root.pointerY - root.height + 64;
            const speed = Math.max(-14, Math.min(14, edge / 4));
            const bottom = Math.max(root.view.originY, root.view.originY + root.view.contentHeight - root.view.height);
            root.view.contentY = Math.max(root.view.originY, Math.min(bottom, root.view.contentY + speed));
            root.updateTarget();
        }
    }

    Connections {
        target: root.entries
        function onRowsAboutToBeRemoved() { root.cancel(); }
        function onModelAboutToBeReset() { root.cancel(); }
    }

    Connections {
        target: root.page
        function onReloadStarted() { root.cancel(); }
        function onFilterChanged() { root.cancel(); }
    }

    onPageChanged: root.cancel()
    onVisibleChanged: {
        if (!visible)
            root.cancel();
    }
}
