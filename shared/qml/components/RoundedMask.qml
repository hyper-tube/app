import QtQuick

ShaderEffect {
    id: root

    property variant source
    property real rounding: 0

    readonly property vector2d itemSize: Qt.vector2d(root.width, root.height)

    fragmentShader: "qrc:/qt/qml/HtMusic/qml/shaders/roundedmask.frag.qsb"
}
