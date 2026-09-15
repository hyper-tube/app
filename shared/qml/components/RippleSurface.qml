import QtQuick
import HtMusic

Item {
    id: root

    property real rounding: Theme.rounding.small
    property color colBackground: "transparent"
    property color colState: Theme.colOnSurface
    property color colRipple: root.colState
    property string tooltip: ""
    property bool tooltipDismissed: false
    property bool interactive: true
    property bool hoverEnabled: true
    property bool rippleEnabled: true
    property int acceptedButtons: Qt.LeftButton | Qt.RightButton
    property real stateOpacity: !root.interactive ? 0
        : tap.pressed ? Theme.state.press
        : (root.hoverEnabled && hover.hovered) ? Theme.state.hover
        : 0

    readonly property alias hovered: hover.hovered
    readonly property alias pressed: tap.pressed

    default property alias content: contentHolder.data

    signal clicked
    signal rightClicked(real x, real y)

    function startRipple(x, y) {
        const squared = (dx, dy) => dx * dx + dy * dy;
        expand.reach = Math.sqrt(Math.max(
            squared(x, y),
            squared(root.width - x, y),
            squared(x, root.height - y),
            squared(root.width - x, root.height - y)));
        ripple.origin = Qt.vector2d(x, y);
        fade.complete();
        expand.restart();
    }

    MaterialToolTip {
        parent: root
        text: root.tooltip
        visible: root.tooltip.length > 0 && root.hovered && root.interactive && !root.tooltipDismissed
            && root.visible && root.enabled && root.opacity > 0 && !root.pressed
    }

    Rectangle {
        anchors.fill: parent
        radius: root.rounding
        color: root.colBackground

        Behavior on color {
            ColorAnimation {
                duration: Theme.duration.fast
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveEffects
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: root.rounding
        color: root.colState
        opacity: root.stateOpacity

        Behavior on opacity {
            NumberAnimation {
                duration: Theme.duration.fast
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveEffects
            }
        }
    }

    Ripple {
        id: ripple

        anchors.fill: parent
        rounding: root.rounding
        colRipple: root.colRipple
    }

    SequentialAnimation {
        id: expand

        property real reach: 0

        PropertyAction { target: ripple; property: "strength"; value: Theme.state.ripple }
        NumberAnimation {
            target: ripple
            property: "reach"
            from: 0
            to: expand.reach
            duration: Theme.duration.ripple
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.standardDecel
        }
    }

    NumberAnimation {
        id: fade

        target: ripple
        property: "strength"
        to: 0
        duration: Theme.duration.rippleFade
        easing.type: Easing.BezierSpline
        easing.bezierCurve: Theme.curve.standardDecel
    }

    HoverHandler {
        id: hover

        enabled: root.interactive
        cursorShape: Qt.PointingHandCursor
    }

    TapHandler {
        id: tap

        enabled: root.interactive
        acceptedButtons: root.acceptedButtons
        gesturePolicy: TapHandler.ReleaseWithinBounds

        onPressedChanged: {
            if (tap.pressed)
                root.tooltipDismissed = true;
            if (!root.rippleEnabled)
                return;
            if (tap.pressed)
                root.startRipple(tap.point.position.x, tap.point.position.y);
            else
                fade.restart();
        }

        onTapped: (eventPoint, button) => {
            if (button === Qt.RightButton)
                root.rightClicked(eventPoint.position.x, eventPoint.position.y);
            else
                root.clicked();
        }
    }

    Item {
        id: contentHolder

        anchors.fill: parent
    }
    onHoveredChanged: {
        if (!root.hovered)
            root.tooltipDismissed = false;
    }
}
