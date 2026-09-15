import QtQuick
import QtQuick.Effects
import HtMusic

Item {
    id: root

    required property BrowseModel page

    property bool inline: false
    property bool expanded: false
    property real expansion: root.expanded ? 1 : 0

    readonly property var entry: root.page ? root.page.entry : null
    readonly property bool writable: Account.signedIn && Connectivity.online
    readonly property bool sideways: root.inline && root.width >= 560
    readonly property real coverSize: root.sideways ? Math.round(Math.min(232, root.width * 0.3))
        : Math.min(root.width - 24, root.inline ? 240 : 288)
    readonly property int alignment: root.sideways ? Text.AlignLeft : Text.AlignHCenter
    readonly property bool clipped: fullDescription.implicitHeight > shortDescription.implicitHeight + 1
    readonly property bool bylined: !!root.page && root.page.byline.length > 0

    signal menuRequested(var request)

    implicitHeight: root.sideways ? Math.max(cover.height, details.y + details.implicitHeight)
        : details.y + details.implicitHeight

    Behavior on expansion {
        NumberAnimation {
            duration: Theme.duration.spatial
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.emphasized
        }
    }

    RippleSurface {
        id: byline

        property real reveal: root.bylined ? 1 : 0

        x: root.sideways ? details.x - 10 : (root.width - width) / 2
        implicitWidth: bylineRow.implicitWidth + 20
        implicitHeight: 34
        opacity: byline.reveal
        scale: 0.9 + 0.1 * byline.reveal
        transformOrigin: root.sideways ? Item.Left : Item.Center
        rounding: Theme.rounding.full
        interactive: !!root.entry && root.bylined && root.entry.actions.profileId.length > 0
        Accessible.role: Accessible.Button
        Accessible.name: root.page ? root.page.byline : ""

        Row {
            id: bylineRow

            anchors.centerIn: parent
            spacing: 8

            Artwork {
                anchors.verticalCenter: parent.verticalCenter
                width: 22
                height: 22
                visible: !!root.page && root.page.bylineArtId.length > 0
                artId: root.page ? root.page.bylineArtId : ""
                rounding: Theme.rounding.full
            }

            StyledText {
                anchors.verticalCenter: parent.verticalCenter
                text: root.page ? root.page.byline : ""
                font.pixelSize: Theme.font.smallie
                color: Theme.colOnSurfaceVariant
            }
        }

        Behavior on reveal {
            NumberAnimation {
                duration: Theme.duration.fast
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveEffects
            }
        }

        onClicked: Browser.openPage(root.entry.actions.profileId, "profile")
    }

    Item {
        id: cover

        x: root.sideways ? 0 : (root.width - width) / 2
        y: root.sideways ? 0 : byline.height + 16
        width: root.coverSize
        height: root.coverSize

        RectangularShadow {
            anchors.fill: art
            radius: Theme.rounding.large
            blur: 36
            spread: 2
            offset: Qt.vector2d(0, 10)
            color: ColorUtils.withAlpha(Theme.colShadow, 0.42)
        }

        Artwork {
            id: art

            anchors.fill: parent
            artId: root.page ? root.page.artId : ""
            rounding: Theme.rounding.large
        }
    }

    Column {
        id: details

        x: root.sideways ? cover.width + 28 : 0
        y: root.sideways ? byline.height + 10 : cover.y + cover.height + 16
        width: root.width - details.x
        spacing: root.sideways ? 12 : 16

        StyledText {
            width: parent.width
            text: root.page ? root.page.title : ""
            title: true
            horizontalAlignment: root.alignment
            font.pixelSize: Theme.font.display
            color: Theme.colOnSurface
            wrapMode: Text.WordWrap
            maximumLineCount: 3
            elide: Text.ElideRight
        }

        Item {
            width: parent.width
            height: shortDescription.implicitHeight
                + (fullDescription.implicitHeight - shortDescription.implicitHeight) * root.expansion
            visible: !!root.page && root.page.description.length > 0
            clip: true

            StyledText {
                id: shortDescription

                width: parent.width
                text: root.page ? root.page.description : ""
                textFormat: Text.PlainText
                horizontalAlignment: root.alignment
                font.pixelSize: Theme.font.smallie
                color: Theme.colOnSurfaceVariant
                wrapMode: Text.WordWrap
                maximumLineCount: 3
                elide: Text.ElideRight
                opacity: 1 - root.expansion
            }

            StyledText {
                id: fullDescription

                width: parent.width
                text: root.page ? root.page.description : ""
                textFormat: Text.PlainText
                horizontalAlignment: root.alignment
                font.pixelSize: Theme.font.smallie
                color: Theme.colOnSurfaceVariant
                wrapMode: Text.WordWrap
                opacity: root.expansion
            }
        }

        PillButton {
            x: root.sideways ? -12 : (details.width - width) / 2
            visible: root.clipped
            ghost: true
            horizontalPadding: 12
            text: root.expanded ? qsTr("Less") : qsTr("More")
            icon: root.expanded ? "expand_less" : "expand_more"
            onClicked: root.expanded = !root.expanded
        }

        Row {
            x: root.sideways ? 0 : (details.width - width) / 2
            spacing: 8

            PillButton {
                anchors.verticalCenter: parent.verticalCenter
                interactive: !!root.page && root.page.playable
                opacity: interactive ? 1 : 0.4
                text: qsTr("Play")
                icon: "play_arrow"
                toggled: true
                onClicked: Browser.playPage()
            }

            PillButton {
                anchors.verticalCenter: parent.verticalCenter
                visible: root.writable && !!root.page && root.page.collectable
                text: root.page && root.page.saved ? qsTr("Saved") : qsTr("Save")
                icon: root.page && root.page.saved ? "bookmark_added" : "bookmark_add"
                onClicked: root.page.toggleSaved()
            }

            IconButton {
                anchors.verticalCenter: parent.verticalCenter
                icon: "share"
                Accessible.name: qsTr("Share")
                onClicked: Actions.share(root.entry)
            }

            IconButton {
                id: overflow

                anchors.verticalCenter: parent.verticalCenter
                icon: "more_vert"
                Accessible.name: qsTr("More actions")
                onClicked: {
                    const spot = root.mapFromItem(overflow, overflow.width / 2, overflow.height);
                    root.menuRequested({ "entry": root.entry, "index": -1, "model": null,
                                         "source": root, "x": spot.x, "y": spot.y, "selectable": false });
                }
            }
        }
    }

    onPageChanged: root.expanded = false
}
