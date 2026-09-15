import QtQuick
import HtMusic

Item {
    id: root

    readonly property bool populated: EpisodeDetails.description.length > 0

    signal podcastRequested(string browseId)

    BusySpinner {
        anchors.centerIn: parent
        visible: EpisodeDetails.loading && !root.populated
    }

    ErrorState {
        anchors.fill: parent
        size: ErrorState.Pane
        shown: EpisodeDetails.failed && !root.populated
        busy: Connectivity.checking || EpisodeDetails.loading
        icon: EpisodeDetails.unreachable ? "cloud_off" : "error"
        title: EpisodeDetails.unreachable ? qsTr("Details need a connection")
            : qsTr("Couldn't load the details")
        caption: EpisodeDetails.unreachable ? qsTr("They show up here as soon as you're back online.")
            : qsTr("Try again in a moment.")
        onActionTriggered: EpisodeDetails.retry()
    }

    Flickable {
        id: scroller

        anchors.fill: parent
        anchors.rightMargin: 12
        contentWidth: width
        contentHeight: column.implicitHeight + 24
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        opacity: root.populated ? 1 : 0
        visible: opacity > 0

        Behavior on opacity {
            NumberAnimation {
                duration: Theme.duration.fast
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveEffects
            }
        }

        Column {
            id: column

            width: scroller.width
            spacing: 12

            StyledText {
                width: parent.width
                visible: EpisodeDetails.meta.length > 0
                text: EpisodeDetails.meta
                font.pixelSize: Theme.font.smallie
                color: Theme.colOnSurfaceVariant
                elide: Text.ElideRight
            }

            StyledText {
                width: parent.width
                text: EpisodeDetails.title
                title: true
                font.pixelSize: Theme.font.huge
                color: Theme.colOnSurface
                wrapMode: Text.WordWrap
            }

            RippleSurface {
                implicitWidth: podcastRow.implicitWidth + 20
                implicitHeight: 40
                visible: EpisodeDetails.podcast.length > 0
                rounding: Theme.rounding.full
                colBackground: ColorUtils.withAlpha(Theme.colLayer3, 0.55)
                interactive: EpisodeDetails.podcastId.length > 0
                Accessible.role: Accessible.Button
                Accessible.name: EpisodeDetails.podcast

                Row {
                    id: podcastRow

                    anchors.centerIn: parent
                    spacing: 8

                    Sym {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "podcasts"
                        iconSize: 18
                        color: Theme.colPrimary
                    }

                    StyledText {
                        anchors.verticalCenter: parent.verticalCenter
                        text: EpisodeDetails.podcast
                        title: true
                        font.pixelSize: Theme.font.smallie
                    }

                    Sym {
                        anchors.verticalCenter: parent.verticalCenter
                        visible: EpisodeDetails.podcastId.length > 0
                        text: "chevron_right"
                        iconSize: 18
                        color: Theme.colOnSurfaceVariant
                    }
                }

                onClicked: root.podcastRequested(EpisodeDetails.podcastId)
            }

            Item {
                width: parent.width
                height: 4
            }

            StyledText {
                id: body

                width: parent.width
                text: EpisodeDetails.description
                textFormat: Text.StyledText
                font.pixelSize: Theme.font.small
                color: Theme.colOnSurface
                linkColor: Theme.colPrimary
                wrapMode: Text.WordWrap
                lineHeight: 1.25
                onLinkActivated: link => EpisodeDetails.activate(link)

                HoverHandler {
                    cursorShape: body.hoveredLink.length > 0 ? Qt.PointingHandCursor : Qt.ArrowCursor
                }
            }
        }
    }

    Item {
        anchors.fill: scroller

        ScrollWheel {
            target: scroller
        }
    }
}
