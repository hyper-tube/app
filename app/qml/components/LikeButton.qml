import QtQuick
import HtMusic

IconButton {
    id: root

    property string videoId: ""
    property bool liked: false
    property bool disliked: false
    property bool down: false

    readonly property bool selected: down ? disliked : liked
    readonly property bool canRate: Account.signedIn && Connectivity.online && root.videoId.length > 0
        && LibraryActions.pendingRatings.indexOf(root.videoId) < 0

    icon: root.down ? "thumb_down" : "thumb_up"
    Accessible.name: root.down ? (root.selected ? qsTr("Remove dislike") : qsTr("Dislike"))
        : (root.selected ? qsTr("Remove like") : qsTr("Like"))
    iconFill: root.selected ? 1 : 0
    colIcon: root.selected ? Theme.colPrimary : Theme.colOnSurfaceVariant
    interactive: root.canRate
    opacity: interactive ? 1 : 0.4

    onClicked: LibraryActions.rateTrack(root.videoId, root.selected ? 0 : root.down ? -1 : 1)
}
