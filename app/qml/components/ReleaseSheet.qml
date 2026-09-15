import QtQuick
import QtQuick.Controls
import HtMusic

ModalSheet {
    id: root

    property real bloom: 0
    property real activity: 0
    property real failure: root.failed ? 1 : 0
    property real spin: 0

    readonly property bool downloading: Updater.stage === Updater.Downloading
    readonly property bool installing: Updater.stage === Updater.Installing
    readonly property bool failed: Updater.stage === Updater.Failed
    readonly property bool busy: root.downloading || root.installing
    readonly property bool structured: Updater.sections.length > 0
    readonly property real notesCeiling: 440
    readonly property real notesHeight: Math.min(root.notesCeiling, notes.implicitHeight + 36)

    readonly property string updateLabel: qsTr("Update now")
    readonly property string cancelLabel: qsTr("Cancel")
    readonly property string restartLabel: qsTr("Restarting")
    readonly property string retryLabel: qsTr("Try again")
    readonly property string downloadLabel: qsTr("Download")

    function stage(index) {
        return Math.max(0, Math.min(1, (root.bloom - index * 0.05) / 0.55));
    }

    title: qsTr("Update available")
    sheetWidth: Math.min(680, root.width - 96)
    sheetHeight: Math.min(root.height - 64,
        root.headerHeight + hero.height + 16 + root.notesHeight + footer.height)

    Connections {
        target: Updater

        function onReleaseChanged() {
            if (!Updater.available)
                root.closeRequested();
        }
    }

    Item {
        id: hero

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.leftMargin: 24
        anchors.right: parent.right
        anchors.rightMargin: 24
        height: 96

        CookieShape {
            id: cookie

            anchors.verticalCenter: parent.verticalCenter
            width: 80
            height: 80
            colFill: Theme.colPrimaryContainer
            opacity: root.stage(0)
            scale: 0.55 + 0.45 * root.bloom
            rotation: root.spin - 60 * (1 - root.bloom)
        }

        Image {
            anchors.centerIn: cookie
            width: 42
            height: 42
            sourceSize.width: 42
            sourceSize.height: 42
            source: "qrc:/icons/ht-music.svg"
            opacity: root.stage(0)
            scale: 0.4 + 0.6 * root.bloom
        }

        Column {
            anchors.left: cookie.right
            anchors.leftMargin: 20
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2

            StyledText {
                width: parent.width
                text: qsTr("New version")
                title: true
                font.pixelSize: Theme.font.smaller
                color: Theme.colPrimary
                elide: Text.ElideRight
                opacity: root.stage(1)
                transform: Translate { x: 16 * (1 - root.stage(1)) }
            }

            Row {
                spacing: 12
                opacity: root.stage(1)
                transform: Translate { x: 16 * (1 - root.stage(1)) }

                StyledText {
                    anchors.verticalCenter: parent.verticalCenter
                    text: Updater.version
                    title: true
                    font.pixelSize: Theme.font.display
                }

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    visible: Updater.prerelease
                    width: prereleaseLabel.implicitWidth + 20
                    height: 26
                    radius: height / 2
                    color: Theme.colTertiaryContainer

                    StyledText {
                        id: prereleaseLabel

                        anchors.centerIn: parent
                        text: qsTr("Pre-release")
                        title: true
                        font.pixelSize: Theme.font.smaller
                        color: Theme.colOnTertiaryContainer
                    }
                }
            }

            StyledText {
                width: parent.width
                text: Updater.published.length > 0
                    ? qsTr("Released %1. You have %2.").arg(Updater.published).arg(AppInfo.version)
                    : qsTr("You have %1.").arg(AppInfo.version)
                font.pixelSize: Theme.font.smallie
                color: Theme.colOnSurfaceVariant
                elide: Text.ElideRight
                opacity: root.stage(2)
                transform: Translate { x: 16 * (1 - root.stage(2)) }
            }
        }
    }

    Rectangle {
        id: plate

        anchors.top: hero.bottom
        anchors.topMargin: 16
        anchors.left: parent.left
        anchors.leftMargin: 24
        anchors.right: parent.right
        anchors.rightMargin: 24
        anchors.bottom: footer.top
        radius: Theme.rounding.normal
        color: Theme.colLayer2
        opacity: root.stage(2)
        transform: Translate { y: 20 * (1 - root.stage(2)) }

        Flickable {
            id: scroller

            anchors.fill: parent
            contentWidth: width
            contentHeight: notes.implicitHeight + 36
            clip: true
            interactive: contentHeight > height
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: PageScrollBar {}

            Column {
                id: notes

                x: 20
                y: 18
                width: scroller.width - 40
                spacing: 20

                Repeater {
                    model: Updater.sections

                    delegate: Column {
                        id: section

                        required property var modelData
                        required property int index

                        readonly property real arrival: root.stage(3 + Math.min(section.index, 5))

                        width: notes.width
                        spacing: 4
                        opacity: section.arrival
                        transform: Translate { y: 14 * (1 - section.arrival) }

                        Column {
                            id: scopes

                            visible: false

                            Repeater {
                                model: section.modelData.notes

                                delegate: StyledText {
                                    required property var modelData

                                    text: modelData.scope.length > 0 ? modelData.scope
                                        : modelData.breaking ? qsTr("Breaking") : ""
                                    title: true
                                    font.pixelSize: Theme.font.smaller
                                }
                            }
                        }

                        Row {
                            visible: section.modelData.title.length > 0
                            height: 32
                            spacing: 10

                            Rectangle {
                                anchors.verticalCenter: parent.verticalCenter
                                width: 28
                                height: 28
                                radius: Theme.rounding.verysmall
                                color: Theme.colSecondaryContainer

                                Sym {
                                    anchors.centerIn: parent
                                    text: section.modelData.icon
                                    iconSize: 17
                                    fill: 1
                                    color: Theme.colOnSecondaryContainer
                                }
                            }

                            StyledText {
                                anchors.verticalCenter: parent.verticalCenter
                                text: section.modelData.title
                                title: true
                                font.pixelSize: Theme.font.large
                            }
                        }

                        Repeater {
                            model: section.modelData.notes

                            delegate: ReleaseNoteRow {
                                required property var modelData

                                width: section.width
                                note: modelData
                                scoped: section.modelData.scoped
                                scopeWidth: scopes.implicitWidth
                            }
                        }
                    }
                }

                StyledText {
                    width: notes.width
                    visible: !root.structured
                    text: Updater.notes.length > 0 ? Updater.notes
                        : qsTr("No release notes were published for this version.")
                    textFormat: Updater.notes.length > 0 ? Text.MarkdownText : Text.PlainText
                    wrapMode: Text.Wrap
                    color: Updater.notes.length > 0 ? Theme.colOnSurface : Theme.colOnSurfaceVariant
                    linkColor: Theme.colPrimary
                    opacity: root.stage(3)
                    onLinkActivated: link => Qt.openUrlExternally(link)
                }
            }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 28
            radius: plate.radius
            opacity: scroller.atYBeginning ? 0 : 1
            gradient: Gradient {
                GradientStop { position: 0; color: Theme.colLayer2 }
                GradientStop { position: 1; color: ColorUtils.withAlpha(Theme.colLayer2, 0) }
            }

            Behavior on opacity {
                NumberAnimation {
                    duration: Theme.duration.fast
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Theme.curve.expressiveEffects
                }
            }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 36
            radius: plate.radius
            opacity: scroller.atYEnd ? 0 : 1
            gradient: Gradient {
                GradientStop { position: 0; color: ColorUtils.withAlpha(Theme.colLayer2, 0) }
                GradientStop { position: 1; color: Theme.colLayer2 }
            }

            Behavior on opacity {
                NumberAnimation {
                    duration: Theme.duration.fast
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Theme.curve.expressiveEffects
                }
            }
        }
    }

    Item {
        id: footer

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 80
        opacity: root.stage(3)

        Item {
            id: measure

            visible: false

            PillButton {
                id: updateMeasure

                text: root.updateLabel
                icon: "download"
                toggled: true
            }

            PillButton {
                id: cancelMeasure

                text: root.cancelLabel
                icon: "close"
            }

            PillButton {
                id: restartMeasure

                text: root.restartLabel
                icon: "restart_alt"
                toggled: true
            }

            PillButton {
                id: retryMeasure

                text: root.retryLabel
                icon: "refresh"
                toggled: true
            }
        }

        Row {
            id: actions

            anchors.right: parent.right
            anchors.rightMargin: 24
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8

            PillButton {
                text: qsTr("Later")
                ghost: true
                interactive: !root.installing
                opacity: root.installing ? 0.4 : 1
                onClicked: root.closeRequested()
            }

            PillButton {
                id: primary

                readonly property Item natural: !Updater.installable ? null
                    : root.downloading ? cancelMeasure
                    : root.installing ? restartMeasure
                    : root.failed ? retryMeasure
                    : updateMeasure
                readonly property real slot: Math.max(updateMeasure.implicitWidth,
                    cancelMeasure.implicitWidth, restartMeasure.implicitWidth,
                    retryMeasure.implicitWidth)

                width: primary.natural ? primary.slot : implicitWidth
                leadingPadding: primary.natural
                    ? (primary.slot - primary.natural.implicitWidth) / 2 + primary.horizontalPadding
                    : primary.horizontalPadding
                toggled: !root.downloading
                busy: root.installing
                interactive: !root.installing
                text: !Updater.installable ? root.downloadLabel
                    : root.downloading ? root.cancelLabel
                    : root.installing ? root.restartLabel
                    : root.failed ? root.retryLabel
                    : root.updateLabel
                icon: !Updater.installable ? "open_in_new"
                    : root.downloading ? "close"
                    : root.installing ? "restart_alt"
                    : root.failed ? "refresh"
                    : "download"
                onClicked: {
                    if (root.downloading)
                        Updater.cancel();
                    else
                        Updater.install();
                }
            }
        }

        Item {
            id: status

            anchors.left: parent.left
            anchors.leftMargin: 24
            anchors.right: actions.left
            anchors.rightMargin: 24
            anchors.top: parent.top
            anchors.bottom: parent.bottom

            PillButton {
                x: -8
                anchors.verticalCenter: parent.verticalCenter
                visible: opacity > 0 && Updater.changelogUrl.toString().length > 0
                opacity: 1 - root.activity
                interactive: root.activity < 0.5
                ghost: true
                text: qsTr("Full changelog")
                icon: "open_in_new"
                onClicked: Qt.openUrlExternally(Updater.changelogUrl)
            }

            Item {
                anchors.fill: parent
                visible: root.activity > 0
                opacity: root.activity

                transform: Translate { y: 10 * (1 - root.activity) }

                Column {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 8
                    visible: opacity > 0
                    opacity: 1 - root.failure

                    Item {
                        width: parent.width
                        height: statusLabel.implicitHeight

                        StyledText {
                            id: statusLabel

                            anchors.left: parent.left
                            anchors.right: amount.left
                            anchors.rightMargin: 12
                            text: root.installing ? qsTr("Checksum verified. Restarting to install...")
                                : qsTr("Downloading update")
                            title: true
                            font.pixelSize: Theme.font.smallie
                            elide: Text.ElideRight
                        }

                        StyledText {
                            id: amount

                            anchors.right: parent.right
                            anchors.verticalCenter: statusLabel.verticalCenter
                            visible: root.downloading
                            text: Updater.transferred
                            font.pixelSize: Theme.font.smaller
                            color: Theme.colOnSurfaceVariant
                        }
                    }

                    ProgressBar {
                        width: parent.width
                        value: root.installing ? 1 : Updater.progress
                    }
                }

                Item {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    height: failureText.implicitHeight
                    visible: opacity > 0
                    opacity: root.failure

                    FontMetrics {
                        id: failureMetrics

                        font: failureText.font
                    }

                    Sym {
                        id: failureGlyph

                        y: (failureMetrics.height - failureGlyph.height) / 2
                        text: "error"
                        iconSize: 20
                        fill: 1
                        color: Theme.colError
                    }

                    StyledText {
                        id: failureText

                        anchors.left: failureGlyph.right
                        anchors.leftMargin: 10
                        anchors.right: parent.right
                        text: Updater.error
                        wrapMode: Text.WordWrap
                        maximumLineCount: 3
                        elide: Text.ElideRight
                        font.pixelSize: Theme.font.smallie
                        color: Theme.colOnSurfaceVariant
                    }
                }
            }
        }
    }

    FrameAnimation {
        id: spinner

        running: root.busy && root.visible
        onTriggered: root.spin = (root.spin + spinner.frameTime * 40) % 360
    }

    StateGroup {
        states: State {
            name: "shown"
            when: root.visible

            PropertyChanges {
                root.bloom: 1
            }
        }

        transitions: Transition {
            to: "shown"

            NumberAnimation {
                target: root
                property: "bloom"
                duration: Theme.duration.spatialSlow
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.curve.expressiveDefaultSpatial
            }
        }
    }

    StateGroup {
        states: State {
            name: "active"
            when: root.busy || root.failed

            PropertyChanges {
                root.activity: 1
            }
        }

        transitions: [
            Transition {
                to: "active"

                NumberAnimation {
                    target: root
                    property: "activity"
                    duration: Theme.duration.enter
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Theme.curve.emphasizedDecel
                }
            },
            Transition {
                from: "active"

                NumberAnimation {
                    target: root
                    property: "activity"
                    duration: Theme.duration.exit
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Theme.curve.emphasizedAccel
                }
            }
        ]
    }

    Behavior on failure {
        NumberAnimation {
            duration: Theme.duration.fast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveEffects
        }
    }
}
