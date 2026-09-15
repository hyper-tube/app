import QtQuick
import QtQuick.Controls
import HtMusic

ModalSheet {
    id: root

    property string section: "appearance"

    readonly property var sections: [
        { "id": "appearance", "icon": "palette", "label": qsTr("Appearance") },
        { "id": "playback", "icon": "play_circle", "label": qsTr("Playback") },
        { "id": "sound", "icon": "graphic_eq", "label": qsTr("Sound") },
        { "id": "downloads", "icon": "download", "label": qsTr("Downloads") },
        { "id": "system", "icon": "desktop_windows", "label": qsTr("System") },
        { "id": "diagnostics", "icon": "bug_report", "label": qsTr("Diagnostics") },
        { "id": "about", "icon": "info", "label": qsTr("About") }
    ].filter(entry => entry.id !== "system" || SystemSettings.available)
    readonly property var swatches: [
        "#7C83FF", "#6750A4", "#3F7BE0", "#00A0A0",
        "#2E9E62", "#B58900", "#E0673F", "#D5407A"
    ]

    readonly property string transitionCaption:
        PlaybackSettings.transitionMode === PlaybackSettings.TransitionsOff
        ? qsTr("Each track ends before the next one starts")
        : PlaybackSettings.transitionMode === PlaybackSettings.Crossfade
        ? qsTr("Overlap every pair of tracks by the same length")
        : qsTr("Analyze both tracks and pick the transition that fits the pair. A pair without an analysis crossfades instead.")
    readonly property string transitionLengthTitle:
        PlaybackSettings.transitionMode === PlaybackSettings.Smart
        ? qsTr("Longest transition")
        : qsTr("Crossfade length")
    readonly property string transitionLengthCaption: PlaybackSettings.crossfadeSeconds === 0
        ? qsTr("Tracks run straight into each other")
        : PlaybackSettings.transitionMode === PlaybackSettings.Smart
        ? qsTr("No transition runs longer than %n seconds", "", PlaybackSettings.crossfadeSeconds)
        : qsTr("Overlap tracks by %n seconds", "", PlaybackSettings.crossfadeSeconds)

    readonly property string colorSourceCaption: !SystemTheme.available
        ? qsTr("Nothing on this system publishes a palette, so the color below is used instead.")
        : SystemTheme.sources.length > 1
        ? qsTr("Every color source found on this system. Pick the one to follow.")
        : qsTr("Every color source found on this system. The one in use is marked.")

    title: qsTr("Settings")
    sheetHeight: Math.min(root.height - 64,
        Math.max(root.headerHeight + rail.implicitHeight + 20,
            root.headerHeight + pages.implicitHeight + 40))

    onOpenChanged: {
        if (root.open)
            picker.adopt(Appearance.seed);
    }

    Connections {
        target: Appearance

        function onChanged() {
            if (Appearance.followsSystemColor)
                picker.adopt(Appearance.seed);
        }
    }

    Column {
        id: rail

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.leftMargin: 14
        width: 208
        spacing: 4

        Repeater {
            model: root.sections

            delegate: RippleSurface {
                id: tab

                required property var modelData
                required property int index

                readonly property bool selected: root.section === tab.modelData.id

                objectName: tab.modelData.id
                width: rail.width
                implicitHeight: 46
                rounding: Theme.rounding.full
                colBackground: tab.selected ? Theme.colSecondaryContainer : "transparent"
                colState: tab.selected ? Theme.colOnSecondaryContainer : Theme.colOnSurface

                Sym {
                    id: tabGlyph

                    anchors.left: parent.left
                    anchors.leftMargin: 18
                    anchors.verticalCenter: parent.verticalCenter
                    text: tab.modelData.icon
                    iconSize: Theme.font.larger
                    fill: tab.selected ? 1 : 0
                    color: tab.selected ? Theme.colOnSecondaryContainer : Theme.colOnSurfaceVariant
                }

                StyledText {
                    anchors.left: tabGlyph.right
                    anchors.leftMargin: 14
                    anchors.verticalCenter: parent.verticalCenter
                    text: tab.modelData.label
                    title: tab.selected
                    color: tab.selected ? Theme.colOnSecondaryContainer : Theme.colOnSurface
                }

                onClicked: root.section = tab.modelData.id
            }
        }
    }

    Flickable {
        id: view

        anchors.top: parent.top
        anchors.left: rail.right
        anchors.leftMargin: 20
        anchors.right: parent.right
        anchors.rightMargin: 12
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        contentHeight: pages.implicitHeight
        clip: true

        ScrollBar.vertical: PageScrollBar {}

        TapHandler {
            onPressedChanged: {
                if (pressed)
                    picker.clearFocus();
            }
        }

        Item {
            id: pages

            property real reveal: 1

            width: view.width - 16
            implicitHeight: appearance.height + playback.height + sound.height
                + downloads.height + system.height + diagnostics.height + about.height
            opacity: pages.reveal
            y: 12 * (1 - pages.reveal)

            Column {
                id: appearance

                width: parent.width
                visible: root.section === "appearance"
                height: visible ? implicitHeight : 0
                spacing: 4

                SettingRow {
                    width: parent.width
                    title: qsTr("Language")
                    caption: qsTr("Also what YouTube Music answers this client in")
                    controlWidth: 246

                    SelectField {
                        width: 246
                        height: 42
                        options: Localization.options
                        value: Localization.language
                        onPicked: chosen => Localization.language = chosen
                    }
                }

                SettingRow {
                    width: parent.width
                    title: qsTr("Theme")
                    caption: qsTr("System follows what this desktop asks for, light or dark")
                    controlWidth: 246

                    SegmentedTabs {
                        width: 246
                        labels: [qsTr("System"), qsTr("Light"), qsTr("Dark")]
                        current: Appearance.mode
                        onCurrentChanged: Appearance.mode = current
                    }
                }

                SettingRow {
                    width: parent.width
                    visible: WindowChrome.selectable
                    title: qsTr("Window decorations")
                    caption: qsTr("Draw the title bar here, or leave it to the desktop")
                    controlWidth: 246

                    SegmentedTabs {
                        width: 246
                        labels: [qsTr("HyperTube"), qsTr("System", "window decorations")]
                        current: WindowChrome.decorations
                        onCurrentChanged: WindowChrome.decorations = current
                    }
                }

                Item {
                    width: parent.width
                    height: WindowChrome.restartPending ? restart.implicitHeight + 12 : 0
                    clip: true

                    NoticeCard {
                        id: restart

                        width: parent.width
                        icon: "restart_alt"
                        colAccent: Theme.colTertiary
                        title: qsTr("The window frame changes on the next start")
                        caption: qsTr("A window keeps the frame it was created with, so HyperTube Music has to start again to draw the other one.")

                        PillButton {
                            text: qsTr("Restart now")
                            toggled: true
                            onClicked: WindowChrome.relaunch()
                        }
                    }

                    Behavior on height {
                        NumberAnimation {
                            duration: Theme.duration.resize
                            easing.type: Easing.BezierSpline
                            easing.bezierCurve: Theme.curve.emphasized
                        }
                    }
                }

                SettingRow {
                    width: parent.width
                    visible: Appearance.systemColorAvailable
                    title: qsTr("Follow the system color")
                    caption: qsTr("Take the accent from the palette this desktop generates")
                    controlWidth: 52

                    ToggleSwitch {
                        checked: Appearance.followsSystemColor
                        onToggled: value => {
                            if (value)
                                Appearance.followSystemColor();
                            else
                                Appearance.seed = picker.value;
                        }
                    }
                }

                Item {
                    id: colorPanels

                    property real transition: Appearance.followsSystemColor ? 0 : 1

                    readonly property bool pinnedNotice: !Appearance.systemColorAvailable
                    readonly property real noticeReveal: colorPanels.pinnedNotice
                        ? 1 : 1 - colorPanels.transition
                    readonly property real pickerReveal: colorPanels.transition

                    width: parent.width
                    height: noticePane.height + pickerPane.height

                    Item {
                        id: noticePane

                        width: parent.width
                        height: colorPanels.noticeReveal * (notice.implicitHeight + 16)
                        opacity: colorPanels.noticeReveal
                        clip: true

                        NoticeCard {
                            id: notice

                            width: parent.width
                            icon: "palette"
                            title: SystemTheme.available
                                ? qsTr("Colors come from %1").arg(SystemTheme.sourceName)
                                : qsTr("No system color source was found")
                            caption: root.colorSourceCaption

                            Repeater {
                                model: SystemTheme.sources

                                delegate: ColorSourceRow {
                                    width: parent.width
                                }
                            }
                        }
                    }

                    Item {
                        id: pickerPane

                        y: noticePane.height
                        width: parent.width
                        height: colorPanels.pickerReveal * (custom.implicitHeight + 8)
                        opacity: colorPanels.pickerReveal
                        clip: true

                        Column {
                            id: custom

                            readonly property real swatchSize: 40
                            readonly property real swatchSpacing: 9
                            readonly property real paletteWidth: Math.min(custom.width,
                                root.swatches.length * custom.swatchSize
                                    + (root.swatches.length - 1) * custom.swatchSpacing)

                            width: parent.width
                            spacing: 12

                            SettingRow {
                                width: parent.width
                                title: qsTr("Accent color")
                                caption: qsTr("Every other color in the interface is generated from this one")
                            }

                            Flow {
                                width: custom.paletteWidth
                                spacing: custom.swatchSpacing

                                Repeater {
                                    model: root.swatches

                                    delegate: RippleSurface {
                                        id: swatch

                                        required property string modelData

                                        readonly property bool selected: !Appearance.followsSystemColor
                                            && Qt.colorEqual(swatch.modelData, Appearance.seed)

                                        implicitWidth: custom.swatchSize
                                        implicitHeight: custom.swatchSize
                                        rounding: Theme.rounding.full
                                        colBackground: swatch.modelData
                                        colState: "#ffffff"

                                        Rectangle {
                                            anchors.centerIn: parent
                                            width: 18
                                            height: 18
                                            radius: Theme.rounding.full
                                            visible: swatch.selected
                                            color: Theme.colLayer1

                                            Sym {
                                                anchors.centerIn: parent
                                                text: "check"
                                                iconSize: 14
                                                color: Theme.colOnSurface
                                            }
                                        }

                                        onClicked: {
                                            picker.clearFocus();
                                            picker.adopt(swatch.modelData);
                                            Appearance.seed = swatch.modelData;
                                        }
                                    }
                                }
                            }

                            ColorPicker {
                                id: picker

                                width: custom.paletteWidth
                                onPicked: chosen => Appearance.seed = chosen
                            }
                        }
                    }

                    Behavior on transition {
                        NumberAnimation {
                            duration: Theme.duration.resize
                            easing.type: Easing.BezierSpline
                            easing.bezierCurve: Theme.curve.emphasized
                        }
                    }
                }
            }

            Column {
                id: playback

                width: parent.width
                visible: root.section === "playback"
                height: visible ? implicitHeight : 0
                spacing: 4

                SettingRow {
                    width: parent.width
                    title: qsTr("Autoplay")
                    caption: qsTr("Keep playing similar music when the queue runs out")
                    controlWidth: 52

                    ToggleSwitch {
                        checked: PlaybackSettings.autoplay
                        onToggled: value => PlaybackSettings.autoplay = value
                    }
                }

                SettingRow {
                    width: parent.width
                    title: qsTr("Play videos by default")
                    caption: qsTr("Open the music video instead of the artwork when a track has one")
                    controlWidth: 52

                    ToggleSwitch {
                        checked: PlaybackSettings.playVideos
                        onToggled: value => PlaybackSettings.playVideos = value
                    }
                }

                SettingRow {
                    width: parent.width
                    title: qsTr("Normalize loudness")
                    caption: qsTr("Even out the volume difference between tracks")
                    controlWidth: 52

                    ToggleSwitch {
                        checked: PlaybackSettings.normalizeLoudness
                        onToggled: value => PlaybackSettings.normalizeLoudness = value
                    }
                }

                SettingRow {
                    width: parent.width
                    title: qsTr("Transitions")
                    caption: root.transitionCaption
                    controlWidth: 246

                    SegmentedTabs {
                        id: modes

                        width: 246
                        labels: [qsTr("Off"), qsTr("Crossfade", "transition mode"), qsTr("Smart")]
                        current: PlaybackSettings.transitionMode
                        onCurrentChanged: {
                            PlaybackSettings.transitionMode = modes.current;
                            modes.current = Qt.binding(() => PlaybackSettings.transitionMode);
                        }
                    }
                }

                Item {
                    id: transitionPanels

                    property real lengthReveal:
                        PlaybackSettings.transitionMode === PlaybackSettings.TransitionsOff ? 0 : 1
                    property real tempoReveal:
                        PlaybackSettings.transitionMode === PlaybackSettings.Smart ? 1 : 0

                    width: parent.width
                    height: lengthPane.height + tempoPane.height

                    Item {
                        id: lengthPane

                        width: parent.width
                        height: transitionPanels.lengthReveal * (lengthRow.implicitHeight + 4)
                        opacity: transitionPanels.lengthReveal
                        clip: true

                        SettingRow {
                            id: lengthRow

                            width: parent.width
                            title: root.transitionLengthTitle
                            caption: root.transitionLengthCaption
                            controlWidth: 220

                            SeekBar {
                                width: 220
                                continuous: true
                                position: PlaybackSettings.crossfadeSeconds
                                duration: PlaybackSettings.maximumCrossfadeSeconds
                                onSeeked: seconds => PlaybackSettings.crossfadeSeconds = Math.round(seconds)
                            }
                        }
                    }

                    Item {
                        id: tempoPane

                        y: lengthPane.height
                        width: parent.width
                        height: transitionPanels.tempoReveal * (tempoRow.implicitHeight + 4)
                        opacity: transitionPanels.tempoReveal
                        clip: true

                        SettingRow {
                            id: tempoRow

                            width: parent.width
                            title: qsTr("Match tempo")
                            caption: qsTr("Stretch the incoming track so both beat grids run together")
                            controlWidth: 52

                            ToggleSwitch {
                                checked: PlaybackSettings.matchTempo
                                Accessible.name: qsTr("Match the tempo of the incoming track")
                                onToggled: value => PlaybackSettings.matchTempo = value
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

                    Behavior on tempoReveal {
                        NumberAnimation {
                            duration: Theme.duration.resize
                            easing.type: Easing.BezierSpline
                            easing.bezierCurve: Theme.curve.emphasized
                        }
                    }
                }
            }

            Column {
                id: sound

                width: parent.width
                visible: root.section === "sound"
                height: visible ? implicitHeight : 0
                spacing: 4

                SettingRow {
                    width: parent.width
                    title: qsTr("Equalizer")
                    caption: qsTr("A ten band graphic equalizer on the playback filter chain")
                    controlWidth: 52

                    ToggleSwitch {
                        checked: PlaybackSettings.equalizerEnabled
                        onToggled: value => PlaybackSettings.equalizerEnabled = value
                    }
                }

                Item {
                    width: parent.width
                    height: equalizer.implicitHeight + 16
                    opacity: PlaybackSettings.equalizerEnabled ? 1 : 0.35
                    enabled: PlaybackSettings.equalizerEnabled

                    Behavior on opacity {
                        NumberAnimation {
                            duration: Theme.duration.fast
                            easing.type: Easing.BezierSpline
                            easing.bezierCurve: Theme.curve.expressiveEffects
                        }
                    }

                    Column {
                        id: equalizer

                        width: parent.width
                        spacing: 16

                        Grid {
                            id: presets

                            width: parent.width
                            columns: 3
                            spacing: 8

                            Repeater {
                                model: PlaybackSettings.presets

                                delegate: PillButton {
                                    required property string modelData
                                    required property int index

                                    width: (presets.width - presets.spacing * (presets.columns - 1))
                                        / presets.columns
                                    text: modelData
                                    toggled: PlaybackSettings.preset === index
                                    onClicked: PlaybackSettings.preset = index
                                }
                            }
                        }

                        Row {
                            width: parent.width
                            height: 190

                            Repeater {
                                model: PlaybackSettings.bands

                                delegate: BandSlider {
                                    required property string modelData
                                    required property int index

                                    width: parent.width / PlaybackSettings.bands.length
                                    height: parent.height
                                    label: modelData
                                    range: PlaybackSettings.gainRange
                                    value: PlaybackSettings.gains[index]
                                    onMoved: level => PlaybackSettings.setGain(index, level)
                                }
                            }
                        }
                    }
                }
            }

            DownloadSettings {
                id: downloads

                width: parent.width
                visible: root.section === "downloads"
                height: visible ? implicitHeight : 0
            }

            Column {
                id: system

                width: parent.width
                visible: root.section === "system"
                height: visible ? implicitHeight : 0
                spacing: 4

                SettingRow {
                    width: parent.width
                    visible: SystemSettings.background !== SystemSettings.Unavailable
                    title: SystemSettings.background === SystemSettings.Dock
                        ? qsTr("Keep running in the Dock") : qsTr("Keep running in the tray")
                    caption: SystemSettings.background === SystemSettings.Dock
                        ? qsTr("Closing the window leaves %1 playing in the Dock").arg(AppInfo.name)
                        : qsTr("Closing the window leaves %1 playing in the notification area").arg(AppInfo.name)
                    controlWidth: 52

                    ToggleSwitch {
                        checked: SystemSettings.closeToTray
                        onToggled: value => SystemSettings.closeToTray = value
                    }
                }

                SettingRow {
                    width: parent.width
                    visible: SystemSettings.startupAvailable
                    title: qsTr("Launch on system startup")
                    caption: qsTr("Start %1 when you sign in to Windows").arg(AppInfo.name)
                    controlWidth: 52

                    ToggleSwitch {
                        checked: SystemSettings.launchAtSignIn
                        onToggled: value => SystemSettings.launchAtSignIn = value
                    }
                }
            }

            Column {
                id: diagnostics

                width: parent.width
                visible: root.section === "diagnostics"
                height: visible ? implicitHeight : 0
                spacing: 4

                SettingRow {
                    width: parent.width
                    visible: DiagnosticsSettings.available
                    title: qsTr("Share crash reports and diagnostics")
                    caption: qsTr("Send crash reports, errors and recent activity to developers. This helps us identify and fix errors when they occur.")
                    controlWidth: 52

                    ToggleSwitch {
                        checked: DiagnosticsSettings.enabled
                        Accessible.name: qsTr("Share crash reports and diagnostics")
                        onToggled: value => DiagnosticsSettings.enabled = value
                    }
                }

                SettingRow {
                    width: parent.width
                    title: qsTr("Logs")
                    caption: AppInfo.logFile
                    controlWidth: 112

                    PillButton {
                        icon: "folder_open"
                        text: qsTr("Open")
                        onClicked: Qt.openUrlExternally(AppInfo.logFolder)
                    }
                }
            }

            Column {
                id: about

                width: parent.width
                visible: root.section === "about"
                height: visible ? implicitHeight : 0
                spacing: 4

                SettingRow {
                    width: parent.width
                    title: AppInfo.name
                    caption: qsTr("A native YouTube Music client in C++ and Qt Quick")
                }

                SettingRow {
                    width: parent.width
                    title: qsTr("Version %1").arg(AppInfo.version)
                    caption: Updater.checkState === Updater.Checking ? qsTr("Checking for updates...")
                        : Updater.available ? qsTr("Version %1 is available").arg(Updater.version)
                        : Updater.checkState === Updater.Unreachable ? qsTr("Could not check for updates")
                        : Updater.checkState === Updater.Checked ? qsTr("You have the latest version")
                        : ""
                    controlWidth: updateAction.implicitWidth

                    PillButton {
                        id: updateAction

                        text: Updater.available ? qsTr("See what's new") : qsTr("Check now")
                        icon: Updater.available ? "arrow_forward" : "refresh"
                        toggled: Updater.available
                        busy: Updater.checkState === Updater.Checking
                        interactive: Updater.checkState !== Updater.Checking
                        onClicked: {
                            if (Updater.available)
                                Updater.showDetails();
                            else
                                Updater.check();
                        }
                    }
                }

                SettingRow {
                    width: parent.width
                    title: qsTr("Check for updates automatically")
                    caption: qsTr("Look for a new version shortly after %1 starts and every few hours").arg(AppInfo.name)
                    controlWidth: 52

                    ToggleSwitch {
                        checked: Updater.automatic
                        Accessible.name: qsTr("Check for updates automatically")
                        onToggled: value => Updater.automatic = value
                    }
                }

                SettingRow {
                    width: parent.width
                    title: qsTr("Website")
                    caption: AppInfo.website
                    controlWidth: 112

                    PillButton {
                        icon: "open_in_new"
                        text: qsTr("Open")
                        onClicked: Qt.openUrlExternally(AppInfo.website)
                    }
                }

                SettingRow {
                    width: parent.width
                    title: qsTr("Source code")
                    caption: AppInfo.repository
                    controlWidth: 112

                    PillButton {
                        icon: "open_in_new"
                        text: qsTr("Open")
                        onClicked: Qt.openUrlExternally(AppInfo.repository)
                    }
                }
            }
        }
    }

    NumberAnimation {
        id: sectionFade

        target: pages
        property: "reveal"
        from: 0
        to: 1
        duration: Theme.duration.fast
        easing.type: Easing.BezierSpline
        easing.bezierCurve: Theme.curve.expressiveEffects
    }

    onSectionChanged: {
        view.contentY = 0;
        sectionFade.restart();
    }
}
