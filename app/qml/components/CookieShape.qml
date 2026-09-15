import QtQuick
import QtQuick.Shapes
import HtMusic

Item {
    id: root

    property int sides: 9
    property real amplitude: Math.min(root.width, root.height) / 36
    property color colFill: Theme.colPrimaryContainer

    readonly property int steps: 216

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            strokeWidth: -1
            strokeColor: "transparent"
            fillColor: root.colFill

            PathPolyline {
                path: {
                    const points = [];
                    const centerX = root.width / 2;
                    const centerY = root.height / 2;
                    const radius = Math.min(root.width, root.height) / 2 - root.amplitude;
                    for (let step = 0; step <= root.steps; ++step) {
                        const angle = step / root.steps * Math.PI * 2;
                        const reach = radius
                            + Math.cos(angle * root.sides) * root.amplitude;
                        points.push(Qt.point(centerX + Math.cos(angle) * reach,
                                             centerY + Math.sin(angle) * reach));
                    }
                    return points;
                }
            }
        }
    }
}
