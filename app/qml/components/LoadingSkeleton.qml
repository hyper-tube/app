import QtQuick
import HtMusic

Column {
    id: root

    property string shape: "list"
    property int columns: 1
    property real cellSize: 168
    property real departure: 0
    property real sweep: -0.3

    readonly property bool shelves: root.shape === "shelves"
    readonly property real inset: Theme.size.cardInset

    spacing: root.shape === "grid" ? 2 : 18
    opacity: 1 - root.departure
    scale: 1 - 0.06 * root.departure
    transformOrigin: Item.Top
    transform: Translate { y: -18 * root.departure }

    Repeater {
        model: root.shape === "episodes" ? 5 : 0

        delegate: Row {
            width: root.width
            spacing: 18
            leftPadding: 14

            SkeletonBlock {
                phase: root.sweep
                width: 96
                height: 96
                radius: Theme.rounding.small
                colBase: Theme.colLayer3
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 10

                SkeletonBlock {
                    phase: root.sweep
                    width: (root.width - 160) * 0.7
                    height: 18
                    radius: Theme.rounding.verysmall
                    colBase: Theme.colLayer4
                }

                SkeletonBlock {
                    phase: root.sweep
                    width: (root.width - 160) * 0.9
                    height: 14
                    radius: Theme.rounding.verysmall
                    colBase: Theme.colLayer3
                }

                SkeletonBlock {
                    phase: root.sweep
                    width: 180
                    height: 12
                    radius: Theme.rounding.verysmall
                    colBase: Theme.colLayer3
                }
            }
        }
    }

    Repeater {
        model: root.shape === "grid" ? 3 : 0

        delegate: Row {
            spacing: 4

            Repeater {
                model: root.columns

                delegate: Item {
                    width: root.cellSize + root.inset * 2
                    height: root.cellSize + 88

                    SkeletonBlock {
                        phase: root.sweep
                        x: root.inset
                        y: root.inset
                        width: root.cellSize
                        height: root.cellSize
                        radius: Theme.rounding.small
                        colBase: Theme.colLayer3
                    }

                    SkeletonBlock {
                        phase: root.sweep
                        x: root.inset
                        y: root.inset + root.cellSize + 14
                        width: root.cellSize * 0.7
                        height: 12
                        radius: Theme.rounding.verysmall
                        colBase: Theme.colLayer4
                    }

                    SkeletonBlock {
                        phase: root.sweep
                        x: root.inset
                        y: root.inset + root.cellSize + 34
                        width: root.cellSize * 0.45
                        height: 10
                        radius: Theme.rounding.verysmall
                        colBase: Theme.colLayer3
                    }
                }
            }
        }
    }

    Repeater {
        model: root.shelves ? 2 : root.shape === "list" ? 7 : 0

        delegate: Column {
            width: root.width
            spacing: 16

            SkeletonBlock {
                phase: root.sweep
                width: root.shelves ? 180 : parent.width * 0.65
                height: root.shelves ? 20 : 44
                radius: Theme.rounding.verysmall
                colBase: Theme.colLayer4
            }

            Row {
                visible: root.shelves
                spacing: 20

                Repeater {
                    model: Math.max(1, Math.floor(root.width / 188))

                    delegate: SkeletonBlock {
                        phase: root.sweep
                        width: 168
                        height: 190
                        radius: Theme.rounding.small
                        colBase: Theme.colLayer3
                    }
                }
            }
        }
    }

    NumberAnimation on sweep {
        running: root.visible
        loops: Animation.Infinite
        from: -0.3
        to: 1.3
        duration: 1400
        easing.type: Easing.InOutSine
    }
}
