import QtQuick
import HtMusic

Flow {
    id: root

    property var chips: []
    property bool interactive: true
    property bool clearable: true
    property string selectedIcon: "check"
    property var seen: ({})
    property int generation: 0

    readonly property real naturalWidth: {
        root.generation;
        let total = 0;
        let shown = 0;
        const shownCount = Math.min(chipRepeater.count, root.chips.length);
        for (let i = 0; i < shownCount; ++i) {
            const chip = chipRepeater.itemAt(i);
            if (!chip || (root.chips[i].clearing && !root.clearable))
                continue;
            total += chip.implicitWidth;
            ++shown;
        }
        return total + Math.max(0, shown - 1) * root.spacing;
    }

    signal chipClicked(int index)

    function remember() {
        const shown = {};
        for (const chip of root.chips)
            shown[chip.title] = true;
        root.seen = shown;
    }

    spacing: 8

    Repeater {
        id: chipRepeater

        model: root.chips

        delegate: PillButton {
            id: chip

            required property var modelData
            required property int index

            property real appear: 0

            readonly property bool clearing: chip.modelData.clearing

            text: chip.modelData.title
            icon: chip.clearing ? "close" : chip.modelData.selected ? root.selectedIcon : ""
            iconSize: chip.clearing ? 18 : Theme.font.normal
            toggled: chip.modelData.selected && !chip.clearing
            horizontalPadding: chip.clearing ? 10 : 16
            leadingPadding: chip.clearing ? 10 : chip.icon.length > 0 ? 12 : 16
            visible: !chip.clearing || root.clearable
            interactive: root.interactive
            opacity: chip.appear
            scale: 0.86 + 0.14 * chip.appear
            transform: Translate { y: 10 * (1 - chip.appear) }
            Accessible.name: chip.clearing ? qsTr("Clear filters") : chip.modelData.title

            SequentialAnimation {
                id: entrance

                PauseAnimation {
                    duration: Math.min(chip.index, 8) * 28
                }

                NumberAnimation {
                    target: chip
                    property: "appear"
                    to: 1
                    duration: Theme.duration.spatialFast
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Theme.curve.expressiveFastSpatial
                }
            }

            Component.onCompleted: {
                if (root.seen[chip.modelData.title] === true)
                    chip.appear = 1;
                else
                    entrance.start();
            }
            onClicked: root.chipClicked(chip.index)
        }

        onItemAdded: root.generation++
        onItemRemoved: root.generation++
    }

    onChipsChanged: Qt.callLater(root.remember)
}
