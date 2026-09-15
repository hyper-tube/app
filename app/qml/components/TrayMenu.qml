import QtQuick
import QtQuick.Effects
import QtQuick.Window
import HtMusic

Window {
    id: root

    property bool open: false

    readonly property real shadowMargin: 34
    readonly property real rounding: Theme.rounding.normal
    readonly property real inset: 6
    readonly property real gap: 2
    readonly property real itemPadding: 14
    readonly property real itemGlyphSize: Theme.font.larger
    readonly property real itemHeight: 44
    readonly property real dividerHeight: 13
    readonly property real minimumWidth: 236

    readonly property var entries: [
        { "icon": PlaybackController.playing ? "pause" : "play_arrow",
          "text": PlaybackController.playing ? qsTr("Pause") : qsTr("Play"),
          "action": "toggle" },
        { "icon": "skip_previous", "text": qsTr("Previous"), "action": "previous" },
        { "icon": "skip_next", "text": qsTr("Next"), "action": "next" },
        { "icon": "desktop_windows", "text": qsTr("Show or hide window"),
          "action": "window", "divided": true },
        { "icon": "power_settings_new", "text": qsTr("Quit"), "action": "quit" }
    ]

    readonly property real plateWidth: Math.max(root.minimumWidth,
        labels.implicitWidth + root.itemGlyphSize + root.itemPadding * 3 + root.inset * 2)
    readonly property real plateHeight: column.implicitHeight + root.inset * 2
    readonly property real columnWidth: root.plateWidth - root.inset * 2

    readonly property bool dropUp: TrayIcon.anchor.y - root.plateHeight >= TrayIcon.anchorArea.y
    readonly property bool dropLeft: TrayIcon.anchor.x - root.plateWidth >= TrayIcon.anchorArea.x
    readonly property real plateLeft: Math.max(TrayIcon.anchorArea.x,
        Math.min(root.dropLeft ? TrayIcon.anchor.x - root.plateWidth : TrayIcon.anchor.x,
            TrayIcon.anchorArea.x + TrayIcon.anchorArea.width - root.plateWidth))
    readonly property real plateTop: Math.max(TrayIcon.anchorArea.y,
        Math.min(root.dropUp ? TrayIcon.anchor.y - root.plateHeight : TrayIcon.anchor.y,
            TrayIcon.anchorArea.y + TrayIcon.anchorArea.height - root.plateHeight))

    function run(action) {
        switch (action) {
        case "toggle":
            PlaybackController.toggle();
            break;
        case "previous":
            PlaybackController.previous();
            break;
        case "next":
            PlaybackController.next();
            break;
        case "window":
            TrayIcon.toggleWindow();
            break;
        case "quit":
            TrayIcon.quit();
            break;
        }
    }

    x: root.plateLeft - root.shadowMargin
    y: root.plateTop - root.shadowMargin
    width: root.plateWidth + root.shadowMargin * 2
    height: root.plateHeight + root.shadowMargin * 2
    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.NoDropShadowWindowHint
    color: "transparent"
    transientParent: null

    Connections {
        target: TrayIcon

        function onMenuRequested() {
            root.open = true;
            TrayIcon.present(root);
        }
    }

    Column {
        id: labels

        visible: false

        Repeater {
            model: root.entries

            delegate: StyledText {
                required property var modelData

                text: modelData.text
            }
        }
    }

    Item {
        id: body

        property real reveal: 0

        anchors.fill: parent
        focus: true
        opacity: Math.min(1, body.reveal * 2.4)
        scale: 0.94 + 0.06 * body.reveal
        transformOrigin: root.dropUp ? Item.Bottom : Item.Top

        Keys.onEscapePressed: root.open = false

        RectangularShadow {
            anchors.fill: clipper
            radius: root.rounding
            blur: 28
            spread: 1
            offset: Qt.vector2d(0, 4)
            color: ColorUtils.withAlpha(Theme.colShadow, 0.34)
        }

        Item {
            id: clipper

            x: root.shadowMargin
            y: root.shadowMargin + (root.dropUp ? root.plateHeight - clipper.height : 0)
            width: root.plateWidth
            height: root.plateHeight * (0.42 + 0.58 * body.reveal)
            clip: true

            Rectangle {
                id: plate

                y: root.dropUp ? clipper.height - root.plateHeight : 0
                width: root.plateWidth
                height: root.plateHeight
                radius: root.rounding
                color: Theme.colLayer3

                Column {
                    id: column

                    x: root.inset
                    y: root.inset
                    width: root.columnWidth
                    spacing: root.gap

                    Repeater {
                        model: root.entries

                        delegate: Item {
                            id: entry

                            required property var modelData

                            readonly property bool divided: entry.modelData.divided === true

                            width: root.columnWidth
                            implicitHeight: (entry.divided ? root.dividerHeight : 0) + root.itemHeight

                            Item {
                                x: -root.inset
                                width: root.plateWidth
                                height: root.dividerHeight
                                visible: entry.divided

                                Rectangle {
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: parent.width
                                    height: 1
                                    color: Theme.colOutlineVariant
                                }
                            }

                            RippleSurface {
                                y: entry.divided ? root.dividerHeight : 0
                                width: root.columnWidth
                                implicitHeight: root.itemHeight
                                rounding: root.rounding - root.inset

                                Sym {
                                    id: glyph

                                    anchors.left: parent.left
                                    anchors.leftMargin: root.itemPadding
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: entry.modelData.icon
                                    iconSize: root.itemGlyphSize
                                    color: Theme.colOnSurfaceVariant
                                }

                                StyledText {
                                    anchors.left: glyph.right
                                    anchors.leftMargin: root.itemPadding
                                    anchors.right: parent.right
                                    anchors.rightMargin: root.itemPadding
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: entry.modelData.text
                                    elide: Text.ElideRight
                                }

                                onClicked: {
                                    root.open = false;
                                    root.run(entry.modelData.action);
                                }
                            }
                        }
                    }
                }
            }
        }

        states: State {
            name: "open"
            when: root.open

            PropertyChanges {
                target: body
                reveal: 1
            }
        }

        transitions: [
            Transition {
                to: "open"

                NumberAnimation {
                    target: body
                    property: "reveal"
                    duration: Theme.duration.spatialFast
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Theme.curve.expressiveFastSpatial
                }
            },
            Transition {
                from: "open"

                SequentialAnimation {
                    NumberAnimation {
                        target: body
                        property: "reveal"
                        duration: Theme.duration.exit
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: Theme.curve.emphasizedAccel
                    }

                    ScriptAction { script: root.hide() }
                }
            }
        ]
    }

    onActiveChanged: {
        if (!root.active)
            root.open = false;
    }
}
