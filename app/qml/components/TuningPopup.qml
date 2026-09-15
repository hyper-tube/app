import QtQuick
import HtMusic

Popover {
    id: root

    readonly property real contentInset: 14
    readonly property real innerWidth: root.columnWidth - root.contentInset * 2
    readonly property string lengthTitle:
        PlaybackSettings.transitionMode === PlaybackSettings.Smart
        ? qsTr("Longest transition")
        : qsTr("Crossfade length")
    readonly property string lengthLabel: qsTr("%1s").arg(PlaybackSettings.crossfadeSeconds)

    width: 452
    gap: 6

    Item {
        width: root.columnWidth
        height: 42

        StyledText {
            anchors.left: parent.left
            anchors.leftMargin: root.contentInset
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Sound")
            title: true
            font.pixelSize: Theme.font.large
        }

        PillButton {
            anchors.right: parent.right
            anchors.rightMargin: root.contentInset - 4
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Reset")
            ghost: true
            horizontalPadding: 12
            onClicked: {
                PlaybackSettings.resetEqualizer();
                PlaybackSettings.transitionMode = PlaybackSettings.TransitionsOff;
                PlaybackSettings.crossfadeSeconds = 0;
            }
        }
    }

    Item {
        id: transitionBlock

        property real lengthReveal:
            PlaybackSettings.transitionMode === PlaybackSettings.TransitionsOff ? 0 : 1

        x: root.contentInset
        width: root.innerWidth
        height: modes.y + modes.height + lengthPane.height

        StyledText {
            id: heading

            text: qsTr("Transitions")
            title: true
        }

        SegmentedTabs {
            id: modes

            y: heading.height + 8
            width: parent.width
            height: 40
            labels: [qsTr("Off"), qsTr("Crossfade", "transition mode"), qsTr("Smart")]
            current: PlaybackSettings.transitionMode
            onCurrentChanged: {
                PlaybackSettings.transitionMode = modes.current;
                modes.current = Qt.binding(() => PlaybackSettings.transitionMode);
            }
        }

        Item {
            id: lengthPane

            y: modes.y + modes.height
            width: parent.width
            height: transitionBlock.lengthReveal * (length.implicitHeight + 12)
            opacity: transitionBlock.lengthReveal
            clip: true

            Column {
                id: length

                y: 12
                width: parent.width
                spacing: 4

                Item {
                    width: parent.width
                    height: 22

                    StyledText {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.lengthTitle
                        title: true
                    }

                    StyledText {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.lengthLabel
                        font.pixelSize: Theme.font.smaller
                        color: Theme.colOnSurfaceVariant
                    }
                }

                SeekBar {
                    width: parent.width
                    continuous: true
                    position: PlaybackSettings.crossfadeSeconds
                    duration: PlaybackSettings.maximumCrossfadeSeconds
                    onSeeked: seconds => PlaybackSettings.crossfadeSeconds = Math.round(seconds)
                }
            }
        }

        Behavior on lengthReveal {
            NumberAnimation {
                duration: Theme.duration.resize
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.emphasized
            }
        }
    }

    Item {
        x: -root.inset
        width: root.width
        height: 15

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width
            height: 1
            color: Theme.colOutlineVariant
        }
    }

    Item {
        width: root.columnWidth
        height: 40

        StyledText {
            anchors.left: parent.left
            anchors.leftMargin: root.contentInset
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Equalizer")
            title: true
        }

        ToggleSwitch {
            anchors.right: parent.right
            anchors.rightMargin: root.contentInset
            anchors.verticalCenter: parent.verticalCenter
            checked: PlaybackSettings.equalizerEnabled
            onToggled: value => PlaybackSettings.equalizerEnabled = value
        }
    }

    Item {
        width: root.columnWidth
        height: bands.height + presets.height + 14
        opacity: PlaybackSettings.equalizerEnabled ? 1 : 0.35
        enabled: PlaybackSettings.equalizerEnabled

        Behavior on opacity {
            NumberAnimation {
                duration: Theme.duration.fast
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveEffects
            }
        }

        Grid {
            id: presets

            x: root.contentInset
            width: root.innerWidth
            columns: 3
            spacing: 8

            Repeater {
                model: PlaybackSettings.presets

                delegate: PillButton {
                    required property string modelData
                    required property int index

                    width: (presets.width - presets.spacing * (presets.columns - 1)) / presets.columns
                    text: modelData
                    toggled: PlaybackSettings.preset === index
                    onClicked: PlaybackSettings.preset = index
                }
            }
        }

        Row {
            id: bands

            anchors.top: presets.bottom
            anchors.topMargin: 14
            anchors.horizontalCenter: parent.horizontalCenter
            height: 170

            Repeater {
                model: PlaybackSettings.bands

                delegate: BandSlider {
                    required property string modelData
                    required property int index

                    width: root.innerWidth / PlaybackSettings.bands.length
                    label: modelData
                    range: PlaybackSettings.gainRange
                    value: PlaybackSettings.gains[index]
                    onMoved: level => PlaybackSettings.setGain(index, level)
                }
            }
        }
    }
}
