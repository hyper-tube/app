import QtQuick
import QtQuick.Window
import HtMusic

Item {
    id: root

    property var options: []
    property string value: ""
    property string label: ""
    property string icon: ""

    readonly property int selectedIndex: root.indexOf(root.value)
    readonly property var selected: root.selectedIndex >= 0 ? root.options[root.selectedIndex] : null

    signal picked(string value)

    implicitWidth: Theme.sized(260)
    implicitHeight: Theme.sized(52)

    function indexOf(value) {
        for (let i = 0; i < root.options.length; ++i) {
            if (root.options[i].value === value)
                return i;
        }
        return -1;
    }

    RippleSurface {
        id: field

        anchors.fill: parent
        rounding: Theme.shape.field

        Rectangle {
            anchors.fill: parent
            radius: field.rounding
            color: "transparent"
            border.width: menu.opened ? 2 : 1
            border.color: menu.opened ? Theme.colPrimary : Theme.colOutlineVariant

            Behavior on border.color {
                ColorAnimation {
                    duration: Theme.duration.fast
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Theme.curve.expressiveEffects
                }
            }
        }

        Rectangle {
            x: Theme.spaced(12)
            y: -7
            width: caption.implicitWidth + Theme.spaced(8)
            height: Theme.sized(14)
            visible: root.label.length > 0
            color: Theme.colLayer1

            StyledText {
                id: caption

                anchors.centerIn: parent
                text: root.label
                font.pixelSize: Theme.font.smallest
                color: menu.opened ? Theme.colPrimary : Theme.colOnSurfaceVariant
            }
        }

        Sym {
            id: leading

            anchors.left: parent.left
            anchors.leftMargin: Theme.spaced(14)
            anchors.verticalCenter: parent.verticalCenter
            visible: leading.text.length > 0
            width: visible ? leading.iconSize : 0
            text: root.selected && root.selected.icon !== undefined ? root.selected.icon : root.icon
            iconSize: Theme.font.larger
            color: Theme.colOnSurfaceVariant
        }

        StyledText {
            anchors.left: leading.visible ? leading.right : parent.left
            anchors.leftMargin: leading.visible ? Theme.spaced(10) : Theme.spaced(14)
            anchors.right: arrow.left
            anchors.rightMargin: Theme.spaced(8)
            anchors.verticalCenter: parent.verticalCenter
            text: root.selected ? root.selected.label : ""
            title: true
            font.pixelSize: Theme.font.small
            elide: Text.ElideRight
        }

        Sym {
            id: arrow

            anchors.right: parent.right
            anchors.rightMargin: Theme.spaced(14)
            anchors.verticalCenter: parent.verticalCenter
            text: "expand_more"
            iconSize: Theme.font.larger
            color: Theme.colOnSurfaceVariant
            rotation: menu.opened ? 180 : 0

            Behavior on rotation {
                NumberAnimation {
                    duration: Theme.duration.spatialFast
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Theme.curve.expressiveFastSpatial
                }
            }
        }

        onClicked: menu.toggle()
    }

    Popover {
        id: menu

        property bool dropUp: false

        z: 120
        parent: field
        width: root.width
        y: menu.dropUp ? -menu.height - Theme.spaced(6) : root.height + Theme.spaced(6)
        anchorY: menu.dropUp ? 1 : 0

        onAboutToShow: {
            const bottom = root.mapToItem(null, 0, root.height).y;
            menu.dropUp = root.Window.height - bottom < menu.height + Theme.spaced(12);
        }

        Repeater {
            model: root.options

            delegate: RippleSurface {
                id: option

                required property var modelData
                required property int index

                readonly property bool current: option.index === root.selectedIndex

                width: menu.columnWidth
                implicitHeight: option.modelData.caption ? Theme.sized(58) : Theme.sized(46)
                rounding: Math.max(0, menu.rounding - menu.inset)
                colBackground: option.current ? Theme.colSecondaryContainer : "transparent"
                colState: option.current ? Theme.colOnSecondaryContainer : Theme.colOnSurface

                Sym {
                    id: glyph

                    anchors.left: parent.left
                    anchors.leftMargin: Theme.spaced(14)
                    anchors.verticalCenter: parent.verticalCenter
                    visible: glyph.text.length > 0
                    width: visible ? glyph.iconSize : 0
                    text: option.modelData.icon !== undefined ? option.modelData.icon : ""
                    iconSize: Theme.font.larger
                    fill: option.current ? 1 : 0
                    color: option.current ? Theme.colOnSecondaryContainer : Theme.colOnSurfaceVariant
                }

                Column {
                    anchors.left: glyph.visible ? glyph.right : parent.left
                    anchors.leftMargin: glyph.visible ? Theme.spaced(10) : Theme.spaced(14)
                    anchors.right: tick.left
                    anchors.rightMargin: Theme.spaced(8)
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: Theme.spaced(2)

                    StyledText {
                        width: parent.width
                        text: option.modelData.label
                        title: option.current
                        font.pixelSize: Theme.font.small
                        color: option.current ? Theme.colOnSecondaryContainer : Theme.colOnSurface
                        elide: Text.ElideRight
                    }

                    StyledText {
                        width: parent.width
                        visible: text.length > 0
                        text: option.modelData.caption !== undefined ? option.modelData.caption : ""
                        font.pixelSize: Theme.font.smaller
                        color: Theme.colInactive
                        elide: Text.ElideRight
                    }
                }

                Sym {
                    id: tick

                    anchors.right: parent.right
                    anchors.rightMargin: Theme.spaced(14)
                    anchors.verticalCenter: parent.verticalCenter
                    visible: option.current
                    width: visible ? tick.iconSize : 0
                    text: "check"
                    iconSize: Theme.font.normal
                    color: Theme.colOnSecondaryContainer
                }

                onClicked: {
                    menu.close();
                    root.picked(option.modelData.value);
                }
            }
        }
    }
}
