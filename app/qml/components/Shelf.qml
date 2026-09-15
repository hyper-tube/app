import QtQuick
import HtMusic

Column {
    id: root

    property string title: ""
    property string subtitle: ""
    property var cards: []
    property bool videoCovers: false
    property real cardSize: 168
    property bool moreAvailable: false
    property bool moreGhost: false

    readonly property real inset: Theme.size.cardInset
    readonly property real stride: root.cardSize + root.inset * 2 + strip.spacing
    readonly property bool scrollable: strip.contentWidth > strip.width

    signal cardActivated(int index)
    signal cardPlayRequested(int index)
    signal cardMenuRequested(int index, Item source, real x, real y)
    signal moreRequested
    signal endReached

    spacing: 16

    Item {
        width: parent.width
        visible: root.title.length > 0 || root.subtitle.length > 0 || arrows.visible
        height: visible ? implicitHeight : 0
        implicitHeight: Math.max(header.implicitHeight, arrows.implicitHeight)

        SectionHeader {
            id: header

            anchors.left: parent.left
            anchors.right: arrows.left
            anchors.rightMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            title: root.title
            subtitle: root.subtitle
        }

        Row {
            id: arrows

            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2
            visible: root.scrollable || root.moreAvailable

            PillButton {
                anchors.verticalCenter: parent.verticalCenter
                visible: root.moreAvailable
                ghost: root.moreGhost
                text: qsTr("See all")
                onClicked: root.moreRequested()
            }

            IconButton {
                anchors.verticalCenter: parent.verticalCenter
                icon: "chevron_left"
                Accessible.name: qsTr("Scroll left")
                visible: root.scrollable
                iconSize: 24
                interactive: !strip.atXBeginning
                opacity: strip.atXBeginning ? 0.3 : 1
                onClicked: glide.scrollBy(-1)

                Behavior on opacity {
                    NumberAnimation {
                        duration: Theme.duration.fast
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: Theme.curve.expressiveEffects
                    }
                }
            }

            IconButton {
                anchors.verticalCenter: parent.verticalCenter
                icon: "chevron_right"
                Accessible.name: qsTr("Scroll right")
                visible: root.scrollable
                iconSize: 24
                interactive: !strip.atXEnd
                opacity: strip.atXEnd ? 0.3 : 1
                onClicked: glide.scrollBy(1)

                Behavior on opacity {
                    NumberAnimation {
                        duration: Theme.duration.fast
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: Theme.curve.expressiveEffects
                    }
                }
            }
        }
    }

    Item {
        x: -root.inset
        width: root.width + root.inset * 2
        height: root.cardSize + root.inset * 2 + 52

        ListView {
            id: strip

            anchors.fill: parent
            orientation: ListView.Horizontal
            spacing: 2
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            acceptedButtons: Qt.NoButton
            cacheBuffer: 0
            reuseItems: true
            onContentXChanged: {
                if (contentX + width + 320 >= originX + contentWidth)
                    root.endReached();
            }
            model: root.cards

            delegate: Card {
                id: card

                required property var entry
                required property int index

                title: entry.title
                subtitle: entry.subtitle
                artId: entry.artId
                circular: entry.circular
                video: root.videoCovers && entry.track.video
                playable: entry.playable
                pinned: entry.pinned
                live: entry.track.live
                active: entry.track.valid
                    && PlaybackController.track.videoId === entry.track.videoId
                artSize: root.cardSize
                onClicked: root.cardActivated(index)
                onPlayRequested: root.cardPlayRequested(index)
                onMenuRequested: (x, y) => root.cardMenuRequested(card.index, card, x, y)
            }
        }
    }

    ScrollGlide {
        id: glide

        target: strip
        step: Math.max(1, Math.floor(strip.width / root.stride)) * root.stride
    }
}
