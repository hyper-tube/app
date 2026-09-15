import QtQuick
import QtQuick.Controls
import HtMusic

Item {
    id: root

    property Plugin plugin: null

    readonly property bool present: root.plugin !== null
    readonly property bool live: root.present && root.plugin.enabled
    readonly property bool custom: root.present && root.plugin.info.settingsSource.length > 0
    readonly property bool configurable: root.custom
        || (root.present && root.plugin.settings.length > 0)
    readonly property bool speaking: root.present && root.plugin.status.length > 0
    readonly property int health: root.present ? root.plugin.health : Plugin.Ok

    readonly property string noticeIcon: root.health === Plugin.Error ? "error"
        : root.health === Plugin.Warning ? "warning"
        : root.health === Plugin.Busy ? "sync"
        : "check_circle"
    readonly property string noticeTitle: root.health === Plugin.Error ? qsTr("Not working")
        : root.health === Plugin.Warning ? qsTr("Needs attention")
        : root.health === Plugin.Busy ? qsTr("Working on it")
        : qsTr("Working")
    readonly property color colNotice: root.health === Plugin.Error ? Theme.colError
        : root.health === Plugin.Warning ? Theme.colTertiary
        : Theme.colPrimary

    property real noticeReveal: root.speaking ? 1 : 0

    onPluginChanged: tabs.current = 0
    onVisibleChanged: {
        if (!root.visible)
            tabs.current = 0;
    }

    Behavior on noticeReveal {
        NumberAnimation {
            duration: Theme.duration.spatialFast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.curve.expressiveFastSpatial
        }
    }

    Item {
        id: hero

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.leftMargin: 24
        anchors.right: parent.right
        anchors.rightMargin: 24
        height: 84

        Rectangle {
            id: tile

            anchors.verticalCenter: parent.verticalCenter
            width: 68
            height: 68
            radius: Theme.rounding.normal
            color: root.live ? Theme.colPrimaryContainer : Theme.colLayer3

            Behavior on color {
                ColorAnimation {
                    duration: Theme.duration.fast
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Theme.curve.expressiveEffects
                }
            }

            PluginGlyph {
                anchors.centerIn: parent
                info: root.present ? root.plugin.info : null
                glyphSize: 32
                fill: root.live ? 1 : 0
                colGlyph: root.live ? Theme.colOnPrimaryContainer : Theme.colOnSurfaceVariant
            }
        }

        Column {
            anchors.left: tile.right
            anchors.leftMargin: 18
            anchors.right: power.left
            anchors.rightMargin: 18
            anchors.verticalCenter: parent.verticalCenter
            spacing: 4

            StyledText {
                width: parent.width
                text: root.present ? root.plugin.info.name : ""
                title: true
                font.pixelSize: Theme.font.larger
                elide: Text.ElideRight
            }

            StyledText {
                width: parent.width
                text: root.present ? root.plugin.info.description : ""
                font.pixelSize: Theme.font.smallie
                color: Theme.colOnSurfaceVariant
                wrapMode: Text.WordWrap
            }
        }

        ToggleSwitch {
            id: power

            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            checked: root.live
            Accessible.name: qsTr("Enable this plugin")
            onToggled: value => root.plugin.enabled = value
        }
    }

    MaterialTabs {
        id: tabs

        anchors.top: hero.bottom
        anchors.topMargin: 8
        anchors.left: parent.left
        anchors.leftMargin: 24
        anchors.right: parent.right
        anchors.rightMargin: 24
        visible: root.configurable
        labels: [qsTr("Home"), qsTr("Settings")]
    }

    Item {
        id: panes

        anchors.top: root.configurable ? tabs.bottom : hero.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        clip: true

        Flickable {
            id: home

            x: -width * tabs.position
            width: parent.width
            height: parent.height
            contentHeight: overview.implicitHeight + 34
            contentWidth: width
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: PageScrollBar {}

            Column {
                id: overview

                x: 24
                y: 18
                width: home.width - 60
                spacing: 18

                Item {
                    width: parent.width
                    height: notice.implicitHeight * root.noticeReveal
                    clip: true
                    opacity: root.noticeReveal

                    NoticeCard {
                        id: notice

                        width: parent.width
                        y: (parent.height - height) / 2
                        icon: root.noticeIcon
                        colAccent: root.colNotice
                        title: root.noticeTitle
                        caption: root.present ? root.plugin.status : ""
                    }
                }

                StyledText {
                    width: parent.width
                    text: root.present ? root.plugin.info.about : ""
                    textFormat: Text.MarkdownText
                    wrapMode: Text.WordWrap
                    font.pixelSize: Theme.font.smallie
                    color: Theme.colOnSurfaceVariant
                    linkColor: Theme.colPrimary
                    onLinkActivated: link => Qt.openUrlExternally(link)
                }
            }
        }

        Flickable {
            id: preferences

            x: width * (1 - tabs.position)
            width: parent.width
            height: parent.height
            contentHeight: settings.implicitHeight + 34
            contentWidth: width
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: PageScrollBar {}

            Column {
                id: settings

                x: 24
                y: 12
                width: preferences.width - 60
                spacing: 2
                opacity: root.live ? 1 : 0.4
                enabled: root.live

                Behavior on opacity {
                    NumberAnimation {
                        duration: Theme.duration.fast
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: Theme.curve.expressiveEffects
                    }
                }

                Repeater {
                    model: root.present && !root.custom ? root.plugin.settings : []

                    delegate: PluginSettingRow {
                        required property var modelData

                        width: settings.width
                        plugin: root.plugin
                        descriptor: modelData
                    }
                }

                Loader {
                    width: settings.width
                    active: root.custom
                    source: root.custom ? root.plugin.info.settingsSource : ""
                }
            }
        }
    }
}
