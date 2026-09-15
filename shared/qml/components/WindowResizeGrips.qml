import QtQuick
import HtMusic

Item {
    id: root

    readonly property int thickness: WindowChrome.gripThickness
    readonly property int corner: WindowChrome.gripThickness * 2

    visible: WindowChrome.grips

    WindowResizeGrip {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: root.thickness
        edges: Qt.LeftEdge
        cursor: Qt.SizeHorCursor
    }

    WindowResizeGrip {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: root.thickness
        edges: Qt.RightEdge
        cursor: Qt.SizeHorCursor
    }

    WindowResizeGrip {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: root.thickness
        edges: Qt.TopEdge
        cursor: Qt.SizeVerCursor
    }

    WindowResizeGrip {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: root.thickness
        edges: Qt.BottomEdge
        cursor: Qt.SizeVerCursor
    }

    WindowResizeGrip {
        anchors.left: parent.left
        anchors.top: parent.top
        width: root.corner
        height: root.corner
        edges: Qt.LeftEdge | Qt.TopEdge
        cursor: Qt.SizeFDiagCursor
    }

    WindowResizeGrip {
        anchors.right: parent.right
        anchors.top: parent.top
        width: root.corner
        height: root.corner
        edges: Qt.RightEdge | Qt.TopEdge
        cursor: Qt.SizeBDiagCursor
    }

    WindowResizeGrip {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        width: root.corner
        height: root.corner
        edges: Qt.LeftEdge | Qt.BottomEdge
        cursor: Qt.SizeBDiagCursor
    }

    WindowResizeGrip {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        width: root.corner
        height: root.corner
        edges: Qt.RightEdge | Qt.BottomEdge
        cursor: Qt.SizeFDiagCursor
    }
}
