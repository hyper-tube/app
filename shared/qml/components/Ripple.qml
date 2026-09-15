import QtQuick
import HtMusic

ShaderEffect {
    id: root

    property real rounding: 0
    property color colRipple: Theme.colOnSurface
    property real strength: 0
    property real reach: 0
    property vector2d origin: Qt.vector2d(0, 0)

    readonly property vector2d itemSize: Qt.vector2d(root.width, root.height)

    visible: root.strength > 0
    fragmentShader: "qrc:/qt/qml/HtMusic/qml/shaders/ripple.frag.qsb"
}
