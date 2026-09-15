import QtQuick
import HtMusic

Item {
    id: root

    enum Size {
        Page,
        Pane,
        Inline
    }

    property int size: ErrorState.Page
    property bool shown: false
    property bool busy: false
    property string icon: "cloud_off"
    property string title: ""
    property string caption: ""
    property string actionText: qsTr("Try again")
    property string actionIcon: "refresh"
    property string secondaryText: ""
    property string secondaryIcon: ""
    property real reveal: 0
    property real busyReveal: root.busy ? 1 : 0
    property real pulse: 0

    readonly property bool inline: root.size === ErrorState.Inline
    readonly property bool large: root.size === ErrorState.Page
    readonly property real plateSize: root.large ? 96 : root.inline ? 36 : 64
    readonly property real glyphSize: root.large ? 44 : root.inline ? 19 : 30

    signal actionTriggered
    signal secondaryTriggered

    function stage(index) {
        return Math.max(0, Math.min(1, (root.reveal - index * 0.12) / 0.64));
    }

    implicitWidth: root.inline ? root.plateSize + 14 + Math.max(inlineTitle.implicitWidth,
        inlineCaption.implicitWidth) + 12 + inlineAction.implicitWidth : (root.large ? 420 : 320)
    implicitHeight: root.inline ? 52 : block.implicitHeight
    visible: root.reveal > 0
    enabled: root.shown
    Accessible.role: Accessible.AlertMessage
    Accessible.name: root.title

    Behavior on busyReveal {
        NumberAnimation {
            duration: Theme.duration.fast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveEffects
        }
    }

    Item {
        id: plate

        x: root.inline ? 0 : (root.width - width) / 2
        y: root.inline ? (root.height - height) / 2 : block.y
        width: root.plateSize
        height: root.plateSize
        opacity: root.stage(0)
        scale: 0.5 + 0.5 * root.reveal

        Rectangle {
            anchors.fill: parent
            radius: width / 2
            color: "transparent"
            border.width: 2
            border.color: Theme.colPrimary
            visible: root.busyReveal > 0
            opacity: (1 - root.pulse) * 0.6 * root.busyReveal
            scale: 1 + 0.5 * root.pulse
        }

        Rectangle {
            anchors.fill: parent
            radius: width / 2
            color: Theme.colSecondaryContainer
        }

        Sym {
            anchors.centerIn: parent
            text: root.icon
            iconSize: root.glyphSize
            color: Theme.colOnSecondaryContainer
        }
    }

    Column {
        id: block

        x: (root.width - width) / 2
        y: Math.max(0, (root.height - implicitHeight) / 2)
        width: Math.max(0, Math.min(root.large ? 420 : 320, root.width - 32))
        visible: !root.inline
        topPadding: root.plateSize + (root.large ? 24 : 16)
        spacing: 8

        StyledText {
            width: parent.width
            text: root.title
            title: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.pixelSize: root.large ? Theme.font.huge : Theme.font.large
            opacity: root.stage(1)
            transform: Translate { y: 14 * (1 - root.stage(1)) }
        }

        StyledText {
            width: parent.width
            visible: root.caption.length > 0
            text: root.caption
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.pixelSize: root.large ? Theme.font.small : Theme.font.smallie
            color: Theme.colOnSurfaceVariant
            opacity: root.stage(2)
            transform: Translate { y: 14 * (1 - root.stage(2)) }
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            topPadding: root.large ? 16 : 10
            spacing: 8
            opacity: root.stage(3)
            transform: Translate { y: 14 * (1 - root.stage(3)) }

            PillButton {
                visible: root.actionText.length > 0
                text: root.actionText
                icon: root.actionIcon
                toggled: true
                busy: root.busy
                onClicked: root.actionTriggered()
            }

            PillButton {
                visible: root.secondaryText.length > 0
                text: root.secondaryText
                icon: root.secondaryIcon
                onClicked: root.secondaryTriggered()
            }
        }
    }

    Item {
        x: root.plateSize + 14
        width: Math.max(0, root.width - x)
        height: root.height
        visible: root.inline

        Column {
            anchors.left: parent.left
            anchors.right: inlineAction.left
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            spacing: 1
            opacity: root.stage(1)
            transform: Translate { x: 10 * (1 - root.stage(1)) }

            StyledText {
                id: inlineTitle

                width: parent.width
                text: root.title
                title: true
                font.pixelSize: Theme.font.smallie
                elide: Text.ElideRight
            }

            StyledText {
                id: inlineCaption

                width: parent.width
                visible: root.caption.length > 0
                text: root.caption
                font.pixelSize: Theme.font.smaller
                color: Theme.colOnSurfaceVariant
                elide: Text.ElideRight
            }
        }

        PillButton {
            id: inlineAction

            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            text: root.actionText
            icon: root.actionIcon
            busy: root.busy
            opacity: root.stage(2)
            onClicked: root.actionTriggered()
        }
    }

    NumberAnimation on pulse {
        from: 0
        to: 1
        duration: 1400
        loops: Animation.Infinite
        running: root.visible && root.busyReveal > 0
    }

    states: State {
        name: "shown"
        when: root.shown

        PropertyChanges {
            target: root
            reveal: 1
        }
    }

    transitions: [
        Transition {
            to: "shown"

            NumberAnimation {
                property: "reveal"
                duration: Theme.duration.spatialSlow
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveDefaultSpatial
            }
        },
        Transition {
            from: "shown"

            NumberAnimation {
                property: "reveal"
                duration: Theme.duration.exit
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.emphasizedAccel
            }
        }
    ]
}
