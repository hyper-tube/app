import QtQuick
import HtMusic

Rectangle {
    id: root

    property int current: 0
    property bool expanded: false
    property real expansion: expanded ? 1 : 0

    readonly property var entries: [
        { "id": "home", "icon": "home", "label": qsTr("Home") },
        { "id": "explore", "icon": "explore", "label": qsTr("Explore") },
        { "id": "library", "icon": "library_music", "label": qsTr("Library") },
        { "id": "downloads", "icon": "download_for_offline", "label": qsTr("Downloads") }
    ]

    signal navigateRequested(int index)
    signal pluginsRequested
    signal settingsRequested

    implicitWidth: Theme.size.railCollapsed
        + (Theme.size.railExpanded - Theme.size.railCollapsed) * expansion
    color: Theme.colLayer0

    Behavior on expansion {
        NumberAnimation {
            duration: Theme.duration.spatial
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.emphasized
        }
    }

    IconButton {
        id: menuButton

        x: 44 + (40 - 44) * root.expansion - width / 2
        y: 16
        icon: root.expanded ? "menu_open" : "menu"
        Accessible.name: root.expanded ? qsTr("Collapse navigation") : qsTr("Expand navigation")
        iconSize: 24
        onClicked: root.expanded = !root.expanded
    }

    Column {
        anchors.top: menuButton.bottom
        anchors.topMargin: 32
        width: parent.width
        spacing: 4

        Repeater {
            model: root.entries

            delegate: NavItem {
                required property var modelData
                required property int index

                objectName: modelData.id
                width: root.width
                icon: modelData.icon
                label: modelData.label
                selected: root.current === index
                expansion: root.expansion
                onClicked: root.navigateRequested(index)
            }
        }
    }

    Column {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 16
        width: root.width
        spacing: 4

        NavItem {
            objectName: "plugins"
            width: root.width
            icon: "extension"
            label: qsTr("Plugins")
            expansion: root.expansion
            onClicked: root.pluginsRequested()
        }

        NavItem {
            objectName: "settings"
            width: root.width
            icon: "settings"
            label: qsTr("Settings")
            expansion: root.expansion
            onClicked: root.settingsRequested()
        }
    }
}
