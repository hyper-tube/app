import QtQuick
import HtMusic

Item {
    id: root

    required property var entry

    readonly property real gap: 8
    readonly property real rise: 14
    readonly property real threshold: 0.3
    readonly property real settled: Math.max(0, Math.min(1, root.reveal))
    readonly property real swept: Math.min(1, Math.abs(root.offset) / Math.max(1, root.width))

    property bool entered: false
    property bool closing: false
    property int direction: 1
    property real reveal: 0
    property real offset: 0

    function release() {
        if (Math.abs(root.offset) > root.width * root.threshold)
            root.dismiss(root.offset < 0 ? -1 : 1);
        else
            recoil.restart();
    }

    function dismiss(towards) {
        root.direction = towards;
        root.closing = true;
    }

    height: (card.implicitHeight + root.gap) * root.settled

    Item {
        id: plate

        width: parent.width
        height: card.implicitHeight
        x: root.offset
        opacity: root.settled * (1 - root.swept)

        transform: Translate { y: (1 - root.reveal) * -root.rise }

        Rectangle {
            id: card

            width: parent.width
            implicitHeight: Math.max(66, column.implicitHeight + 26)
            radius: Theme.rounding.normal
            color: Theme.colLayer4
            Accessible.role: Accessible.AlertMessage
            Accessible.name: root.entry.title

            Rectangle {
                id: badge

                x: 13
                y: 13
                width: 40
                height: 40
                radius: Theme.rounding.small
                color: root.entry.activity || root.entry.unread ? Theme.colSecondaryContainer
                    : Theme.colLayer3

                Sym {
                    anchors.centerIn: parent
                    text: root.entry.activity ? "download"
                        : root.entry.icon.length > 0 ? root.entry.icon : "campaign"
                    iconSize: 21
                    fill: root.entry.unread ? 1 : 0
                    color: root.entry.activity || root.entry.unread
                        ? Theme.colOnSecondaryContainer : Theme.colOnSurfaceVariant
                }
            }

            Column {
                id: column

                anchors.left: badge.right
                anchors.leftMargin: 12
                anchors.right: parent.right
                anchors.rightMargin: 14
                y: 13
                spacing: 5

                Item {
                    width: parent.width
                    height: heading.implicitHeight

                    StyledText {
                        id: heading

                        anchors.left: parent.left
                        anchors.right: close.left
                        anchors.rightMargin: 6
                        text: root.entry.title
                        title: true
                        elide: Text.ElideRight
                        maximumLineCount: 1
                    }

                    IconButton {
                        id: close

                        anchors.right: parent.right
                        anchors.verticalCenter: heading.verticalCenter
                        icon: "close"
                        iconSize: 17
                        diameter: root.entry.dismissible ? 30 : 0
                        visible: root.entry.dismissible
                        opacity: hover.hovered ? 1 : 0
                        Accessible.name: qsTr("Dismiss notification")
                        onClicked: root.dismiss(1)

                        Behavior on opacity {
                            NumberAnimation {
                                duration: Theme.duration.fast
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: Theme.curve.expressiveEffects
                            }
                        }
                    }
                }

                StyledText {
                    width: parent.width
                    visible: root.entry.body.length > 0
                    text: root.entry.body
                    textFormat: root.entry.activity ? Text.PlainText : Text.MarkdownText
                    wrapMode: Text.WordWrap
                    elide: root.entry.activity ? Text.ElideRight : Text.ElideNone
                    maximumLineCount: root.entry.activity ? 1 : 12
                    font.pixelSize: Theme.font.smallie
                    color: Theme.colOnSurfaceVariant
                    linkColor: Theme.colPrimary
                    onLinkActivated: link => Qt.openUrlExternally(link)
                }

                Item {
                    width: parent.width
                    height: root.entry.activity ? 10 : 0
                    visible: root.entry.activity

                    Rectangle {
                        anchors.bottom: parent.bottom
                        width: parent.width
                        height: 4
                        radius: height / 2
                        color: ColorUtils.withAlpha(Theme.colOnSurface, 0.12)

                        Rectangle {
                            width: parent.width * Math.max(0, Math.min(1, root.entry.progress))
                            height: parent.height
                            radius: parent.radius
                            color: Theme.colPrimary

                            Behavior on width {
                                NumberAnimation {
                                    duration: Theme.duration.fast
                                    easing.type: Easing.OutCubic
                                }
                            }
                        }
                    }
                }

                PillButton {
                    visible: root.entry.action.length > 0
                    text: root.entry.action
                    icon: root.entry.actionIcon.length > 0 ? root.entry.actionIcon : "open_in_new"
                    toggled: true
                    horizontalPadding: 14
                    onClicked: ControlCenter.invokeAction(root.entry.id)
                }

                StyledText {
                    visible: !root.entry.activity && root.entry.age.length > 0
                    text: root.entry.age
                    font.pixelSize: Theme.font.smaller
                    color: Theme.colInactive
                }
            }

            HoverHandler {
                id: hover
            }

            DragHandler {
                id: drag

                target: null
                enabled: root.entry.dismissible && !root.closing
                yAxis.enabled: false

                onActiveChanged: {
                    if (drag.active)
                        recoil.stop();
                    else
                        root.release();
                }
                onActiveTranslationChanged: {
                    if (drag.active)
                        root.offset = drag.activeTranslation.x;
                }
            }
        }
    }

    NumberAnimation {
        id: recoil

        target: root
        property: "offset"
        to: 0
        duration: Theme.duration.spatialFast
        easing.type: Easing.BezierSpline
        easing.bezierCurve: Theme.curve.expressiveFastSpatial
    }

    states: State {
        name: "shown"
        when: root.entered && !root.closing

        PropertyChanges {
            root.reveal: 1
        }
    }

    transitions: [
        Transition {
            to: "shown"
            NumberAnimation {
                property: "reveal"
                duration: Theme.duration.spatialFast
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveFastSpatial
            }
        },
        Transition {
            from: "shown"
            SequentialAnimation {
                NumberAnimation {
                    target: root
                    property: "offset"
                    to: root.direction * root.width
                    duration: Theme.duration.exit
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Theme.curve.emphasizedAccel
                }
                NumberAnimation {
                    target: root
                    property: "reveal"
                    to: 0
                    duration: Theme.duration.spatialFast
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Theme.curve.expressiveFastSpatial
                }
                ScriptAction {
                    script: ControlCenter.dismiss(root.entry.id)
                }
            }
        }
    ]

    Component.onCompleted: root.entered = true
}
