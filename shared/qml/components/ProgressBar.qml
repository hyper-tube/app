import QtQuick
import QtQuick.Shapes
import HtMusic

Item {
    id: root

    property real value: 0
    property bool indeterminate: false
    property real thickness: 4
    property real amplitude: 2.5
    property real wavelength: 26
    property color colTrack: Theme.colSecondaryContainer
    property color colActive: Theme.colPrimary

    readonly property real settled: root.indeterminate ? root.sweep : root.value
    readonly property real activeWidth: Math.max(0, root.width * root.settled - root.gap)
    readonly property real waviness: Math.min(1, Math.max(0, (1 - root.value) * 1.6))

    property real sweep: 0
    property real phase: 0

    readonly property real gap: 6
    readonly property real stop: root.thickness

    implicitHeight: root.thickness + root.amplitude * 2

    Behavior on value {
        NumberAnimation {
            duration: Theme.duration.fast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveEffects
        }
    }

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer
        layer.enabled: true
        layer.samples: 4

        ShapePath {
            strokeColor: root.colActive
            strokeWidth: root.thickness
            capStyle: ShapePath.RoundCap
            fillColor: "transparent"

            PathPolyline {
                path: {
                    const points = [];
                    const middle = root.height / 2;
                    const reach = root.activeWidth;
                    if (reach <= 0)
                        return [Qt.point(0, middle), Qt.point(0, middle)];
                    const step = 2;
                    const swing = root.amplitude * root.waviness;
                    for (let x = 0; x <= reach; x += step) {
                        const ramp = Math.min(1, (reach - x) / 12);
                        const y = middle + Math.sin((x / root.wavelength) * Math.PI * 2
                            + root.phase) * swing * ramp;
                        points.push(Qt.point(x, y));
                    }
                    points.push(Qt.point(reach, middle));
                    return points;
                }
            }
        }

        ShapePath {
            strokeColor: root.colTrack
            strokeWidth: root.thickness
            capStyle: ShapePath.RoundCap
            fillColor: "transparent"

            PathPolyline {
                path: [
                    Qt.point(Math.min(root.width, root.activeWidth + root.gap + root.stop),
                             root.height / 2),
                    Qt.point(root.width - root.stop - root.gap, root.height / 2)
                ]
            }
        }
    }

    Rectangle {
        x: root.width - root.stop
        y: (root.height - root.stop) / 2
        width: root.stop
        height: root.stop
        radius: root.stop / 2
        color: root.colActive
        opacity: root.settled < 0.99 ? 1 : 0

        Behavior on opacity {
            NumberAnimation {
                duration: Theme.duration.fast
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveEffects
            }
        }
    }

    NumberAnimation {
        target: root
        property: "phase"
        from: 0
        to: Math.PI * 2
        duration: 1400
        loops: Animation.Infinite
        running: root.visible && root.waviness > 0.02
    }

    SequentialAnimation {
        running: root.indeterminate && root.visible
        loops: Animation.Infinite

        NumberAnimation {
            target: root
            property: "sweep"
            from: 0.08
            to: 0.92
            duration: Theme.duration.spatialSlow
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.emphasized
        }
        NumberAnimation {
            target: root
            property: "sweep"
            from: 0.92
            to: 0.08
            duration: Theme.duration.spatialSlow
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.emphasized
        }
    }
}
