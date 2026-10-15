import QtQuick
import HtMusic

Column {
    id: root

    property string shape: "list"
    property int columns: 1
    property real cellSize: Theme.size.card
    property real departure: 0
    property real sweep: -0.3

    readonly property bool shelves: root.shape === "shelves"
    readonly property real inset: Theme.spacing.cardInset

    spacing: root.shape === "grid" ? 2 : 18
    opacity: 1 - root.departure
    scale: 1 - 0.06 * root.departure
    transformOrigin: Item.Top
    transform: Translate { y: -18 * root.departure }

    Repeater {
        model: root.shape === "episodes" ? 5 : 0

        delegate: Row {
            width: root.width
            spacing: Theme.spaced(18)
            leftPadding: Theme.spaced(14)

            SkeletonBlock {
                phase: root.sweep
                width: Theme.sized(96)
                height: Theme.sized(96)
                radius: Theme.shape.artwork
                colBase: Theme.colLayer3
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: Theme.spaced(10)

                SkeletonBlock {
                    phase: root.sweep
                    width: (root.width - Theme.spaced(160)) * 0.7
                    height: Theme.sized(18)
                    radius: Theme.rounding.extraSmall
                    colBase: Theme.colLayer4
                }

                SkeletonBlock {
                    phase: root.sweep
                    width: (root.width - Theme.spaced(160)) * 0.9
                    height: Theme.sized(14)
                    radius: Theme.rounding.extraSmall
                    colBase: Theme.colLayer3
                }

                SkeletonBlock {
                    phase: root.sweep
                    width: Theme.sized(180)
                    height: Theme.sized(12)
                    radius: Theme.rounding.extraSmall
                    colBase: Theme.colLayer3
                }
            }
        }
    }

    Repeater {
        model: root.shape === "grid" ? 3 : 0

        delegate: Row {
            spacing: Theme.spaced(4)

            Repeater {
                model: root.columns

                delegate: Item {
                    width: root.cellSize + root.inset * 2
                    height: root.cellSize + Theme.sized(88)

                    SkeletonBlock {
                        phase: root.sweep
                        x: root.inset
                        y: root.inset
                        width: root.cellSize
                        height: root.cellSize
                        radius: Theme.shape.artwork
                        colBase: Theme.colLayer3
                    }

                    SkeletonBlock {
                        phase: root.sweep
                        x: root.inset
                        y: root.inset + root.cellSize + Theme.spaced(14)
                        width: root.cellSize * 0.7
                        height: Theme.sized(12)
                        radius: Theme.rounding.extraSmall
                        colBase: Theme.colLayer4
                    }

                    SkeletonBlock {
                        phase: root.sweep
                        x: root.inset
                        y: root.inset + root.cellSize + Theme.spaced(34)
                        width: root.cellSize * 0.45
                        height: Theme.sized(10)
                        radius: Theme.rounding.extraSmall
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
            spacing: Theme.spaced(16)

            SkeletonBlock {
                phase: root.sweep
                width: root.shelves ? Theme.sized(180) : parent.width * 0.65
                height: root.shelves ? Theme.sized(20) : Theme.sized(44)
                radius: Theme.rounding.extraSmall
                colBase: Theme.colLayer4
            }

            Row {
                visible: root.shelves
                spacing: Theme.spaced(20)

                Repeater {
                    model: Math.max(1, Math.floor(root.width / (root.cellSize + Theme.spaced(20))))

                    delegate: SkeletonBlock {
                        phase: root.sweep
                        width: Theme.sized(168)
                        height: Theme.sized(190)
                        radius: Theme.shape.artwork
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
