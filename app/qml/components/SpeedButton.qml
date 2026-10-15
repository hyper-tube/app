import QtQuick
import HtMusic

RippleSurface {
    id: root

    property bool glass: false

    readonly property real speed: PlaybackSettings.podcastSpeed
    readonly property bool altered: Math.abs(root.speed - 1) > 0.001

    function label(value) {
        return qsTr("%1x").arg(Number(value).toLocaleString(Qt.locale(), "f", value % 1 === 0 ? 0 : 2)
            .replace(/0$/, ""));
    }

    implicitWidth: Math.max(Theme.sized(56), readout.implicitWidth + Theme.spaced(24))
    implicitHeight: Theme.sized(34)
    rounding: Theme.shape.button
    colBackground: root.altered ? Theme.colSecondaryContainer
        : ColorUtils.withAlpha(Theme.colLayer3, root.glass ? 0.55 : 1)
    colState: root.altered ? Theme.colOnSecondaryContainer : Theme.colOnSurface
    tooltip: qsTr("Playback speed")
    acceptedButtons: Qt.LeftButton
    Accessible.role: Accessible.Button
    Accessible.name: qsTr("Playback speed: %1").arg(root.label(root.speed))

    StyledText {
        id: readout

        anchors.centerIn: parent
        text: root.label(root.speed)
        title: true
        font.pixelSize: Theme.font.smallie
        color: root.altered ? Theme.colOnSecondaryContainer : Theme.colOnSurfaceVariant
    }

    Popover {
        id: menu

        parent: root
        x: (root.width - width) / 2
        y: -height - Theme.spaced(12)
        width: Theme.sized(244)
        anchorX: 0.5
        anchorY: 1

        Item {
            width: menu.columnWidth
            height: Theme.sized(44)

            StyledText {
                anchors.left: parent.left
                anchors.leftMargin: Theme.spaced(12)
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Playback speed")
                title: true
            }

            StyledText {
                anchors.right: parent.right
                anchors.rightMargin: Theme.spaced(12)
                anchors.verticalCenter: parent.verticalCenter
                text: root.label(root.speed)
                font.pixelSize: Theme.font.smallie
                color: Theme.colPrimary
            }
        }

        Grid {
            id: options

            x: Theme.spaced(6)
            width: menu.columnWidth - Theme.spaced(12)
            columns: 4
            spacing: Theme.spaced(6)
            bottomPadding: Theme.spaced(6)

            Repeater {
                model: PlaybackSettings.podcastSpeeds

                delegate: RippleSurface {
                    id: option

                    required property real modelData

                    readonly property bool current: Math.abs(option.modelData - root.speed) < 0.001

                    width: (options.width - options.spacing * (options.columns - 1)) / options.columns
                    implicitHeight: Theme.sized(38)
                    rounding: Theme.shape.button
                    colBackground: option.current ? Theme.colPrimary : Theme.colLayer4
                    colState: option.current ? Theme.colOnPrimary : Theme.colOnSurface
                    acceptedButtons: Qt.LeftButton

                    StyledText {
                        anchors.centerIn: parent
                        text: root.label(option.modelData)
                        title: option.current
                        font.pixelSize: Theme.font.smallie
                        color: option.current ? Theme.colOnPrimary : Theme.colOnSurfaceVariant
                    }

                    onClicked: PlaybackSettings.podcastSpeed = option.modelData
                }
            }
        }
    }

    onClicked: menu.toggle()
}
