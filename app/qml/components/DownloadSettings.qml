import QtQuick
import HtMusic

Column {
    id: root

    readonly property real used: Downloads.smartBytes + Downloads.forcedBytes + Downloads.partialBytes
    readonly property real ceiling: Downloads.sizeLimitGb * 1024 * 1024 * 1024

    spacing: 8

    function megabytes(bytes) {
        return (bytes / 1048576).toFixed(1);
    }

    SettingRow {
        width: parent.width
        title: qsTr("Smart downloads")
        caption: qsTr("Keep your most played music available offline. Skips lower a track's priority.")
        controlWidth: 52

        ToggleSwitch {
            checked: Downloads.smartEnabled
            onToggled: value => Downloads.smartEnabled = value
        }
    }

    SettingRow {
        width: parent.width
        title: qsTr("Tracks to keep")
        caption: qsTr("Up to %n smart downloads", "", Downloads.trackLimit)
        controlWidth: 200

        SeekBar {
            width: 200
            continuous: true
            position: Downloads.trackLimit
            duration: 2000
            onSeeked: value => Downloads.trackLimit = Math.max(1, Math.round(value / 25) * 25)
        }
    }

    SettingRow {
        width: parent.width
        title: qsTr("Storage ceiling")
        caption: qsTr("%1 GB. Manual downloads are always kept until you remove them.")
            .arg(Downloads.sizeLimitGb.toFixed(1))
        controlWidth: 200

        SeekBar {
            width: 200
            continuous: true
            position: Downloads.sizeLimitGb
            duration: 20
            onSeeked: value => Downloads.sizeLimitGb = Math.max(0.1, Math.round(value * 10) / 10)
        }
    }

    Rectangle {
        width: parent.width
        height: 104
        radius: Theme.rounding.normal
        color: Theme.colLayer2

        Column {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12

            StyledText {
                text: qsTr("%1 MB used").arg(root.megabytes(root.used))
                title: true
            }

            Rectangle {
                width: parent.width
                height: 8
                radius: 4
                color: Theme.colLayer4
                clip: true

                Rectangle {
                    id: forced

                    width: parent.width * Math.min(1, Downloads.forcedBytes / root.ceiling)
                    height: parent.height
                    radius: 4
                    color: Theme.colTertiary

                    Behavior on width {
                        NumberAnimation { duration: Theme.duration.spatial; easing.type: Easing.OutCubic }
                    }
                }

                Rectangle {
                    x: forced.width
                    width: Math.max(0, parent.width * Math.min(1, root.used / root.ceiling) - forced.width)
                    height: parent.height
                    radius: 4
                    color: Theme.colPrimary
                }
            }

            StyledText {
                text: Downloads.partialBytes > 0
                    ? qsTr("Smart %1 MB  /  Manual %2 MB  /  Incomplete %3 MB")
                        .arg(root.megabytes(Downloads.smartBytes))
                        .arg(root.megabytes(Downloads.forcedBytes))
                        .arg(root.megabytes(Downloads.partialBytes))
                    : qsTr("Smart %1 MB  /  Manual %2 MB")
                        .arg(root.megabytes(Downloads.smartBytes))
                        .arg(root.megabytes(Downloads.forcedBytes))
                font.pixelSize: Theme.font.smaller
                color: Theme.colOnSurfaceVariant
            }
        }
    }

    NoticeCard {
        width: parent.width
        visible: !Connectivity.online && Downloads.pendingCount > 0
        icon: "cloud_off"
        title: qsTr("Downloads are paused")
        caption: qsTr("%n downloads continue when you're back online.", "", Downloads.pendingCount)
    }

    StyledText {
        width: parent.width
        visible: Downloads.pendingCount > 0 && Connectivity.online
        text: qsTr("%n queued or downloading", "", Downloads.pendingCount)
        color: Theme.colOnSurfaceVariant
        font.pixelSize: Theme.font.smallie
    }

    Item {
        width: parent.width
        height: cleanup.implicitHeight

        Row {
            spacing: 8

            PillButton {
                id: cleanup

                text: qsTr("Clean up now")
                icon: "cleaning_services"
                onClicked: {
                    Downloads.cleanUp();
                    Toasts.show(qsTr("Download plan updated"));
                }
            }

            PillButton {
                text: qsTr("Remove all")
                icon: "delete"
                ghost: true
                interactive: root.used > 0 || Downloads.pendingCount > 0
                onClicked: confirmation.open = true
            }
        }

        PillButton {
            anchors.right: parent.right
            text: qsTr("Browse files")
            icon: "folder_open"
            onClicked: Downloads.browseFiles()
        }
    }

    TextPrompt {
        id: confirmation

        title: qsTr("Remove all downloads?")
        message: qsTr("This removes smart and manual downloads and turns off smart downloads. Tracks in use are removed when playback releases them.")
        showInput: false
        confirm: qsTr("Remove all")
        onAccepted: {
            Downloads.removeAll();
            open = false;
        }
        onDismissed: open = false
    }
}
