import QtQuick
import HtMusic

Item {
    id: root

    property int edges: 0
    property int cursor: Qt.ArrowCursor

    HoverHandler {
        cursorShape: root.cursor
    }

    TapHandler {
        gesturePolicy: TapHandler.DragThreshold

        onPressedChanged: {
            if (pressed)
                WindowChrome.startResize(root.edges);
        }
    }
}
