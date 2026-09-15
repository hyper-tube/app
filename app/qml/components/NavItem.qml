import QtQuick
import HtMusic

Item {
    id: root

    property string icon: ""
    property string label: ""
    property bool selected: false
    property real expansion: 0

    readonly property real indicatorX: 16 + (12 - 16) * expansion
    readonly property real indicatorWidth: 56 + (208 - 56) * expansion
    readonly property real glyphCenterX: 44 + (40 - 44) * expansion
    readonly property real labelOpacity: Math.max(0, (expansion - 0.4) / 0.6)

    signal clicked

    implicitHeight: 52

    RippleSurface {
        id: pill

        tooltip: root.expansion < 0.5 ? root.label : ""

        x: root.indicatorX
        anchors.verticalCenter: parent.verticalCenter
        width: root.indicatorWidth
        height: 36
        rounding: Theme.rounding.full
        colBackground: root.selected ? Theme.colSecondaryContainer : "transparent"
        colState: root.selected ? Theme.colOnSecondaryContainer : Theme.colOnSurface

        onClicked: root.clicked()

        Sym {
            id: glyph

            x: root.glyphCenterX - root.indicatorX - width / 2
            anchors.verticalCenter: parent.verticalCenter
            text: root.icon
            iconSize: 24
            fill: root.selected ? 1 : 0
            color: root.selected ? Theme.colOnSecondaryContainer : Theme.colOnSurfaceVariant
        }

        StyledText {
            x: 68 - root.indicatorX
            anchors.verticalCenter: parent.verticalCenter
            width: Math.max(0, pill.width - x - 14)
            visible: root.labelOpacity > 0
            opacity: root.labelOpacity
            text: root.label
            title: root.selected
            font.pixelSize: Theme.font.smallie
            color: root.selected ? Theme.colOnSecondaryContainer : Theme.colOnSurfaceVariant
            elide: Text.ElideRight
        }
    }
}
