import QtQuick
import HtMusic

Item {
    id: root

    required property Item sourceItem
    property real rounding: Theme.rounding.full

    layer.enabled: visible
    layer.effect: RoundedMask {
        rounding: root.rounding
    }

    ShaderEffectSource {
        anchors.fill: parent
        sourceItem: root.sourceItem
        sourceRect: Qt.rect(0, 0, root.sourceItem.width, Math.max(root.height, 96))
        live: root.visible
    }

    Rectangle {
        anchors.fill: parent
        radius: root.rounding
        color: ColorUtils.withAlpha(Theme.colLayer2, 0.4)
        border.width: 1
        border.color: ColorUtils.withAlpha(Theme.colOutline, 0.25)
    }
}
