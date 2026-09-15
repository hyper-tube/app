import QtQuick
import HtMusic

RippleSurface {
    id: root

    property var choices: []
    property int current: -1
    property string heading: ""
    property bool alignRight: false
    property real openness: menu.opened ? 1 : 0
    property real labelPhase: 0
    property string shownTitle: ""
    property string shownIcon: ""

    readonly property var chosen: root.current >= 0 && root.current < root.choices.length
        ? root.choices[root.current] : null
    readonly property real labelPresence: 1 - Math.abs(root.labelPhase)
    readonly property bool glyphed: root.shownIcon.length > 0

    signal picked(int index)

    function adopt() {
        root.shownTitle = root.chosen ? root.chosen.title : "";
        root.shownIcon = root.chosen ? root.chosen.icon : "";
    }

    implicitWidth: content.implicitWidth + (root.glyphed ? 4 : 16) + 14
    implicitHeight: 40
    rounding: Theme.rounding.full
    colBackground: Theme.colSecondaryContainer
    colState: Theme.colOnSecondaryContainer
    Accessible.role: Accessible.ComboBox
    Accessible.name: root.heading.length > 0 ? qsTr("%1: %2").arg(root.heading).arg(root.shownTitle)
        : root.shownTitle

    Behavior on openness {
        NumberAnimation {
            duration: Theme.duration.spatialFast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveFastSpatial
        }
    }

    Row {
        id: content

        x: root.glyphed ? 4 : 16
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: 32
            height: 32
            radius: Theme.rounding.full
            visible: root.glyphed
            color: Theme.colPrimary

            Sym {
                anchors.centerIn: parent
                text: root.shownIcon
                iconSize: 18
                fill: 1
                color: Theme.colOnPrimary
                opacity: root.labelPresence
                scale: 0.6 + 0.4 * root.labelPresence
            }
        }

        Item {
            anchors.verticalCenter: parent.verticalCenter
            width: label.implicitWidth
            height: label.implicitHeight
            clip: true

            StyledText {
                id: label

                text: root.shownTitle
                title: true
                font.pixelSize: Theme.font.smallie
                color: Theme.colOnSecondaryContainer
                opacity: root.labelPresence
                transform: Translate { y: root.labelPhase * 10 }
            }
        }

        Sym {
            anchors.verticalCenter: parent.verticalCenter
            text: "expand_more"
            iconSize: 20
            color: Theme.colOnSecondaryContainer
            rotation: 180 * root.openness
        }
    }

    Popover {
        id: menu

        parent: root
        x: root.alignRight ? root.width - width : 0
        y: root.height + 8
        width: 264
        anchorX: root.alignRight ? 1 : 0
        anchorY: 0

        StyledText {
            width: menu.columnWidth
            visible: root.heading.length > 0
            leftPadding: 14
            topPadding: 8
            bottomPadding: 6
            text: root.heading
            title: true
            font.pixelSize: Theme.font.smallest
            font.capitalization: Font.AllUppercase
            font.letterSpacing: 0.6
            color: Theme.colOnSurfaceVariant
        }

        Repeater {
            model: root.choices

            delegate: RippleSurface {
                id: option

                required property var modelData
                required property int index

                readonly property bool selected: root.current === option.index
                readonly property bool glyphed: option.modelData.icon.length > 0

                width: menu.columnWidth
                implicitHeight: option.glyphed ? 52 : 44
                visible: !option.modelData.clearing
                rounding: menu.rounding - menu.inset
                colBackground: option.selected ? ColorUtils.withAlpha(Theme.colSecondaryContainer, 0.6)
                    : "transparent"

                Rectangle {
                    id: plate

                    anchors.left: parent.left
                    anchors.leftMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    width: option.glyphed ? 36 : 0
                    height: 36
                    radius: Theme.rounding.full
                    visible: option.glyphed
                    color: option.selected ? Theme.colPrimary : Theme.colLayer4

                    Sym {
                        anchors.centerIn: parent
                        text: option.modelData.icon
                        iconSize: 20
                        fill: option.selected ? 1 : 0
                        color: option.selected ? Theme.colOnPrimary : Theme.colOnSurfaceVariant
                    }
                }

                StyledText {
                    anchors.left: plate.right
                    anchors.leftMargin: option.glyphed ? 12 : 6
                    anchors.right: mark.left
                    anchors.rightMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    text: option.modelData.title
                    title: option.selected
                    color: Theme.colOnSurface
                    elide: Text.ElideRight
                }

                RadioMark {
                    id: mark

                    anchors.right: parent.right
                    anchors.rightMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    checked: option.selected
                }

                onClicked: {
                    const chosen = option.index;
                    menu.close();
                    if (!option.selected)
                        root.picked(chosen);
                }
            }
        }
    }

    SequentialAnimation {
        id: relabel

        NumberAnimation {
            target: root
            property: "labelPhase"
            to: -1
            duration: Theme.duration.exit
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.emphasizedAccel
        }

        ScriptAction {
            script: {
                root.adopt();
                root.labelPhase = 1;
            }
        }

        NumberAnimation {
            target: root
            property: "labelPhase"
            to: 0
            duration: Theme.duration.spatialFast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.emphasizedDecel
        }
    }

    Component.onCompleted: root.adopt()

    onChosenChanged: {
        if (!root.chosen || root.chosen.title === root.shownTitle)
            return;
        if (root.shownTitle.length === 0 || root.opacity === 0 || !root.visible) {
            relabel.stop();
            root.labelPhase = 0;
            root.adopt();
            return;
        }
        relabel.restart();
    }

    onClicked: menu.toggle()
}
