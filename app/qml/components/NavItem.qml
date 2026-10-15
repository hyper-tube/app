import QtQuick
import HtMusic

Item {
    id: root

    property string icon: ""
    property string label: ""
    property bool selected: false
    property real expansion: 0

    readonly property real collapsedX: (Theme.size.railCollapsed - Theme.sized(56)) / 2
    readonly property real indicatorX: root.collapsedX + (Theme.spaced(12) - root.collapsedX) * expansion
    readonly property real indicatorWidth: Theme.sized(56)
        + (Theme.size.railExpanded - Theme.spaced(24) - Theme.sized(56)) * expansion
    readonly property real glyphCenterX: Theme.size.railCollapsed / 2
        + (Theme.spaced(12) + Theme.sized(28) - Theme.size.railCollapsed / 2) * expansion
    readonly property real labelOpacity: Math.max(0, (expansion - 0.4) / 0.6)

    signal clicked

    implicitHeight: Theme.sized(52)

    RippleSurface {
        id: pill

        tooltip: root.expansion < 0.5 ? root.label : ""

        x: root.indicatorX
        anchors.verticalCenter: parent.verticalCenter
        width: root.indicatorWidth
        height: Theme.sized(36)
        rounding: Theme.shape.navigation
        colBackground: root.selected ? Theme.colSecondaryContainer : "transparent"
        colState: root.selected ? Theme.colOnSecondaryContainer : Theme.colOnSurface

        onClicked: root.clicked()

        Sym {
            id: glyph

            x: root.glyphCenterX - root.indicatorX - width / 2
            anchors.verticalCenter: parent.verticalCenter
            text: root.icon
            iconSize: Theme.sized(24)
            fill: root.selected ? 1 : 0
            color: root.selected ? Theme.colOnSecondaryContainer : Theme.colOnSurfaceVariant
        }

        StyledText {
            x: Theme.spaced(12) + Theme.sized(56) - root.indicatorX
            anchors.verticalCenter: parent.verticalCenter
            width: Math.max(0, pill.width - x - Theme.spaced(14))
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
