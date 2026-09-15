import QtQuick
import HtMusic

Item {
    id: root

    property bool playing: false
    property real barWidth: 3
    property real barHeight: 18
    property real barSpacing: 3
    property color colBars: Theme.colPrimary

    implicitWidth: root.barWidth * 3 + root.barSpacing * 2 + 6
    implicitHeight: root.barHeight + 6

    Row {
        anchors.centerIn: parent
        spacing: root.barSpacing
        height: root.barHeight

        Repeater {
            model: 3

            delegate: Rectangle {
                id: bar

                required property int index
                property real level: 0.3

                anchors.bottom: parent.bottom
                width: root.barWidth
                height: root.barHeight * level
                radius: root.barWidth / 2
                color: root.colBars

                SequentialAnimation on level {
                    running: root.playing && root.visible
                    loops: Animation.Infinite

                    NumberAnimation {
                        to: 1 - bar.index * 0.12
                        duration: 260 + bar.index * 90
                        easing.type: Easing.InOutSine
                    }

                    NumberAnimation {
                        to: 0.2 + bar.index * 0.1
                        duration: 340 - bar.index * 60
                        easing.type: Easing.InOutSine
                    }
                }
            }
        }
    }
}
