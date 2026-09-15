import QtQuick
import QtQuick.Controls
import HtMusic

ModalSheet {
    id: root

    property string currentId: ""
    property Plugin displayed: null
    property real depth: root.currentId.length > 0 ? 1 : 0

    function show(id) {
        root.currentId = id;
        root.open = true;
    }

    sheetWidth: Math.min(760, root.width - 96)
    sheetHeight: Math.min(root.height - 64, 620)

    onCurrentIdChanged: {
        const found = root.currentId.length > 0 ? PluginRegistry.find(root.currentId) : null;
        if (found)
            root.displayed = found;
    }

    onVisibleChanged: {
        if (!root.visible) {
            root.currentId = "";
            finder.text = "";
        }
    }

    Behavior on depth {
        NumberAnimation {
            duration: Theme.duration.spatial
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.emphasized
        }
    }

    headerContent: [
        IconButton {
            x: 14
            anchors.verticalCenter: parent.verticalCenter
            icon: "arrow_back"
            visible: root.depth > 0.01
            opacity: root.depth
            scale: 0.7 + 0.3 * root.depth
            Accessible.name: qsTr("Back to the plugin list")
            onClicked: root.currentId = ""
        },
        StyledText {
            x: 24 - 30 * root.depth
            width: parent.width - x - 12
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Plugins")
            title: true
            font.pixelSize: Theme.font.huge
            opacity: 1 - root.depth
            elide: Text.ElideRight
        },
        StyledText {
            x: 64 + 30 * (1 - root.depth)
            width: parent.width - x - 12
            anchors.verticalCenter: parent.verticalCenter
            text: root.displayed ? root.displayed.info.name : ""
            title: true
            font.pixelSize: Theme.font.huge
            opacity: root.depth
            elide: Text.ElideRight
        }
    ]

    Item {
        id: levels

        anchors.fill: parent
        clip: true

        Item {
            id: library

            width: parent.width
            height: parent.height
            x: -width * 0.32 * root.depth
            opacity: 1 - root.depth
            visible: root.depth < 0.999
            enabled: root.depth < 0.5

            SearchBar {
                id: finder

                anchors.top: parent.top
                anchors.left: parent.left
                anchors.leftMargin: 24
                anchors.right: parent.right
                anchors.rightMargin: 24
                placeholder: qsTr("Search plugins")
                onTextChanged: PluginRegistry.query = finder.text
                onEditingFinished: list.forceActiveFocus()
            }

            ListView {
                id: list

                anchors.top: finder.bottom
                anchors.topMargin: 14
                anchors.left: parent.left
                anchors.leftMargin: 16
                anchors.right: parent.right
                anchors.rightMargin: 16
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 16
                clip: true
                spacing: 2
                boundsBehavior: Flickable.StopAtBounds
                model: PluginRegistry

                ScrollBar.vertical: PageScrollBar {}

                delegate: PluginRow {
                    width: list.width
                    onOpenRequested: root.currentId = plugin.info.id
                }

                add: Transition {
                    NumberAnimation {
                        property: "opacity"
                        from: 0
                        to: 1
                        duration: Theme.duration.enter
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: Theme.curve.emphasizedDecel
                    }
                    NumberAnimation {
                        property: "scale"
                        from: 0.92
                        to: 1
                        duration: Theme.duration.enter
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: Theme.curve.emphasizedDecel
                    }
                }

                remove: Transition {
                    NumberAnimation {
                        property: "opacity"
                        to: 0
                        duration: Theme.duration.exit
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: Theme.curve.emphasizedAccel
                    }
                    NumberAnimation {
                        property: "scale"
                        to: 0.92
                        duration: Theme.duration.exit
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: Theme.curve.emphasizedAccel
                    }
                }

                displaced: Transition {
                    NumberAnimation {
                        properties: "x,y"
                        duration: Theme.duration.spatialFast
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: Theme.curve.expressiveFastSpatial
                    }
                }
            }

            EmptyState {
                anchors.fill: list
                visible: PluginRegistry.count === 0
                icon: "search_off"
                title: qsTr("No plugins match")
                caption: qsTr("Nothing here is called %1").arg(PluginRegistry.query)
            }
        }

        Item {
            id: page

            width: parent.width
            height: parent.height
            x: width * 0.32 * (1 - root.depth)
            opacity: root.depth
            visible: root.depth > 0.001
            enabled: root.depth > 0.5

            PluginPage {
                anchors.fill: parent
                plugin: root.displayed
            }
        }
    }
}
