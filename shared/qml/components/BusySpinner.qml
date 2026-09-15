import QtQuick
import HtMusic

ShaderEffect {
    id: root

    property bool running: true
    property color colArc: Theme.colPrimary
    property real thickness: Math.max(2, Math.min(root.width, root.height) * 0.115)
    property real progress: 0

    readonly property vector2d itemSize: Qt.vector2d(root.width, root.height)
    readonly property real lead: root.settled(Math.min(1, root.progress * 2))
    readonly property real trail: root.settled(Math.max(0, root.progress * 2 - 1))
    readonly property real head: root.progress + root.trail
    readonly property real sweep: 0.035 + (root.lead - root.trail) * 0.72

    function settled(value) {
        return value * value * (3 - 2 * value);
    }

    implicitWidth: 32
    implicitHeight: 32
    Accessible.role: Accessible.Indicator
    Accessible.name: qsTr("Loading")

    fragmentShader: "qrc:/qt/qml/HtMusic/qml/shaders/spinner.frag.qsb"

    NumberAnimation on progress {
        from: 0
        to: 1
        duration: 1400
        loops: Animation.Infinite
        running: root.running && root.visible && root.opacity > 0
    }
}
