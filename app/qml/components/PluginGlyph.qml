import QtQuick
import QtQuick.Effects
import HtMusic

Item {
    id: root

    property var info: null
    property real glyphSize: Theme.font.larger
    property real fill: 0
    property color colGlyph: Theme.colOnSurfaceVariant

    readonly property bool drawn: root.info !== null && root.info.iconSource.length > 0

    implicitWidth: root.glyphSize
    implicitHeight: root.glyphSize

    Sym {
        anchors.centerIn: parent
        visible: !root.drawn
        text: root.info ? root.info.icon : ""
        iconSize: root.glyphSize
        fill: root.fill
        color: root.colGlyph
    }

    Image {
        anchors.centerIn: parent
        visible: root.drawn
        source: root.drawn ? root.info.iconSource : ""
        sourceSize.width: root.glyphSize
        sourceSize.height: root.glyphSize
        width: root.glyphSize
        height: root.glyphSize
        fillMode: Image.PreserveAspectFit
        smooth: true
        layer.enabled: root.drawn
        layer.effect: MultiEffect {
            colorization: 1
            colorizationColor: root.colGlyph
        }
    }
}
