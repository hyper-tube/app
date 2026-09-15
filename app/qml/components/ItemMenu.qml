import QtQuick
import QtQuick.Controls
import HtMusic

PopupMenu {
    id: root

    property contentItem entry
    property var extraActions: []
    property var builder: null
    property real spotX: 0
    property real spotY: 0

    readonly property real rowHeight: 46

    signal extraTriggered(string action)

    minimumWidth: 268
    modal: true
    dim: false
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    anchorX: root.width > 0 ? Math.max(0, Math.min(1, (root.spotX - root.x) / root.width)) : 0.5
    anchorY: root.height > 0 ? Math.max(0, Math.min(1, (root.spotY - root.y) / root.height)) : 0

    function show(request) {
        const built = (root.builder ? root.builder(request.entry)
            : Actions.build(request.entry, { "selectable": request.selectable, "selectionCount": 0 }))
            .concat(root.extraActions);
        if (built.length === 0 || !root.parent)
            return;
        root.entry = request.entry;
        root.actions = built;
        const spot = root.parent.mapFromItem(request.source, request.x, request.y);
        const reach = built.length * root.rowHeight + root.inset * 2;
        root.spotX = spot.x;
        root.spotY = spot.y;
        root.x = Math.max(0, Math.min(request.above ? spot.x - root.width / 2 : spot.x,
            root.parent.width - root.width));
        root.y = request.above ? Math.max(0, spot.y - reach - 8)
            : Math.max(0, Math.min(spot.y, Math.max(0, root.parent.height - reach)));
        root.open();
    }

    onTriggered: index => {
        if (index < 0 || index >= root.actions.length)
            return;
        const action = root.actions[index].action;
        if (root.extraActions.some(entry => entry.action === action))
            root.extraTriggered(action);
        else
            Actions.run(root.entry, action);
    }
}
