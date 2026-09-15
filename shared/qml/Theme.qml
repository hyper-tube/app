pragma Singleton

import QtQuick
import HtMusic

QtObject {
    id: root

    readonly property bool darkMode: Palette.darkMode

    readonly property color colLayer0: Palette.background
    readonly property color colLayer1: Palette.surfaceContainerLow
    readonly property color colLayer2: Palette.surfaceContainer
    readonly property color colLayer3: Palette.surfaceContainerHigh
    readonly property color colLayer4: Palette.surfaceContainerHighest

    readonly property color colOnLayer0: Palette.onBackground
    readonly property color colOnLayer1: Palette.onSurfaceVariant
    readonly property color colOnLayer2: Palette.onSurface

    readonly property color colOnSurface: Palette.onSurface
    readonly property color colOnSurfaceVariant: Palette.onSurfaceVariant
    readonly property color colInverseSurface: Palette.inverseSurface
    readonly property color colInverseOnSurface: Palette.inverseOnSurface
    readonly property color colInactive: ColorUtils.mix(Palette.onSurfaceVariant, colLayer1, 0.45)

    readonly property color colPrimary: Palette.primary
    readonly property color colOnPrimary: Palette.onPrimary
    readonly property color colPrimaryContainer: Palette.primaryContainer
    readonly property color colOnPrimaryContainer: Palette.onPrimaryContainer

    readonly property color colSecondary: Palette.secondary
    readonly property color colSecondaryContainer: Palette.secondaryContainer
    readonly property color colOnSecondaryContainer: Palette.onSecondaryContainer

    readonly property color colTertiary: Palette.tertiary
    readonly property color colTertiaryContainer: Palette.tertiaryContainer
    readonly property color colOnTertiaryContainer: Palette.onTertiaryContainer

    readonly property color colOutline: Palette.outline
    readonly property color colOutlineVariant: Palette.outlineVariant
    readonly property color colError: Palette.error
    readonly property color colOnError: Palette.onError
    readonly property color colShadow: Palette.shadow
    readonly property color colScrim: Palette.scrim

    readonly property QtObject state: QtObject {
        readonly property real hover: 0.08
        readonly property real press: 0.10
        readonly property real drag: 0.16
        readonly property real ripple: 0.13
    }

    readonly property QtObject rounding: QtObject {
        readonly property int unsharpen: 2
        readonly property int unsharpenmore: 6
        readonly property int verysmall: 8
        readonly property int small: 12
        readonly property int normal: 17
        readonly property int large: 23
        readonly property int verylarge: 30
        readonly property int full: 9999
    }

    readonly property QtObject font: QtObject {
        readonly property string main: "Roboto Flex"
        readonly property string numbers: "Roboto Flex"
        readonly property string icon: "Material Symbols Rounded"
        readonly property string mono: Qt.platform.os === "windows" ? "Consolas"
            : Qt.platform.os === "osx" ? "Menlo" : "monospace"

        readonly property var axes: ({ "wght": 450, "wdth": 100 })
        readonly property var axesTitle: ({ "wght": 550, "wdth": 100 })

        readonly property int smallest: 10
        readonly property int smaller: 12
        readonly property int smallie: 13
        readonly property int small: 15
        readonly property int normal: 16
        readonly property int large: 17
        readonly property int larger: 19
        readonly property int huge: 22
        readonly property int display: 30
    }

    readonly property QtObject curve: QtObject {
        readonly property list<real> expressiveFastSpatial: [0.42, 1.67, 0.21, 0.90, 1, 1]
        readonly property list<real> expressiveDefaultSpatial: [0.38, 1.21, 0.22, 1.00, 1, 1]
        readonly property list<real> expressiveSlowSpatial: [0.39, 1.29, 0.35, 0.98, 1, 1]
        readonly property list<real> expressiveEffects: [0.34, 0.80, 0.34, 1.00, 1, 1]
        readonly property list<real> emphasized: [0.05, 0, 2 / 15, 0.06, 1 / 6, 0.4, 5 / 24, 0.82, 0.25, 1, 1, 1]
        readonly property list<real> emphasizedAccel: [0.3, 0, 0.8, 0.15, 1, 1]
        readonly property list<real> emphasizedDecel: [0.05, 0.7, 0.1, 1, 1, 1]
        readonly property list<real> standard: [0.2, 0, 0, 1, 1, 1]
        readonly property list<real> standardDecel: [0, 0, 0, 1, 1, 1]
    }

    readonly property QtObject duration: QtObject {
        readonly property int fast: 200
        readonly property int spatialFast: 350
        readonly property int spatial: 500
        readonly property int spatialSlow: 650
        readonly property int enter: 400
        readonly property int exit: 200
        readonly property int resize: 300
        readonly property int sheetEnter: 500
        readonly property int sheetExit: 400
        readonly property int ripple: 1000
        readonly property int rippleFade: 600
    }

    readonly property QtObject size: QtObject {
        readonly property int contentMaxWidth: 1620
        readonly property int railCollapsed: 88
        readonly property int railExpanded: 232
        readonly property int playerBar: 88
        readonly property int gutter: 24
        readonly property int cardInset: 10
        readonly property int bleed: 20
    }
}
