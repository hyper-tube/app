pragma Singleton

import QtQuick
import HtMusic

QtObject {
    id: root

    readonly property var tokens: ActiveTheme.values
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
        readonly property real hover: root.tokens["state.hover"]
        readonly property real press: root.tokens["state.press"]
        readonly property real drag: root.tokens["state.drag"]
        readonly property real ripple: root.tokens["state.ripple"]
    }

    readonly property QtObject rounding: QtObject {
        readonly property real extraSmall: root.tokens["shape.scale.extraSmall"]
        readonly property real small: root.tokens["shape.scale.small"]
        readonly property real medium: root.tokens["shape.scale.medium"]
        readonly property real large: root.tokens["shape.scale.large"]
        readonly property real extraLarge: root.tokens["shape.scale.extraLarge"]
        readonly property real full: root.tokens["shape.scale.full"]
    }

    readonly property QtObject shape: QtObject {
        readonly property real sheet: root.tokens["shape.sheet"]
        readonly property real popover: root.tokens["shape.popover"]
        readonly property real tooltip: root.tokens["shape.tooltip"]
        readonly property real card: root.tokens["shape.card"]
        readonly property real row: root.tokens["shape.row"]
        readonly property real field: root.tokens["shape.field"]
        readonly property real searchField: root.tokens["shape.searchField"]
        readonly property real playerBar: root.tokens["shape.playerBar"]
        readonly property real fullPlayer: root.tokens["shape.fullPlayer"]
        readonly property real artwork: root.tokens["shape.artwork"]
        readonly property real thumbnail: root.tokens["shape.thumbnail"]
        readonly property real cover: root.tokens["shape.cover"]
        readonly property real stage: root.tokens["shape.stage"]
        readonly property real avatar: root.tokens["shape.avatar"]
        readonly property real button: root.tokens["shape.button"]
        readonly property real iconButton: root.tokens["shape.iconButton"]
        readonly property real playButton: root.tokens["shape.playButton"]
        readonly property real chip: root.tokens["shape.chip"]
        readonly property real badge: root.tokens["shape.badge"]
        readonly property real toggle: root.tokens["shape.toggle"]
        readonly property real segmented: root.tokens["shape.segmented"]
        readonly property real navigation: root.tokens["shape.navigation"]
        readonly property real track: root.tokens["shape.track"]
        readonly property real toolbar: root.tokens["shape.toolbar"]
        readonly property real checkbox: root.tokens["shape.checkbox"]
        readonly property real swatch: root.tokens["shape.swatch"]
        readonly property real plate: root.tokens["shape.plate"]
    }

    readonly property QtObject elevation: QtObject {
        readonly property real shadow: root.tokens["elevation.shadow"]
        readonly property real outline: root.tokens["elevation.outline"]
    }

    readonly property QtObject font: QtObject {
        readonly property string main: root.tokens["typography.family"]
        readonly property string icon: root.tokens["icons.family"]
        readonly property string mono: Qt.platform.os === "windows" ? "Consolas"
            : Qt.platform.os === "osx" ? "Menlo" : "monospace"

        readonly property int weight: root.tokens["typography.weight"]
        readonly property int titleWeight: root.tokens["typography.titleWeight"]
        readonly property var axes: ({
            "wght": root.tokens["typography.weight"],
            "wdth": root.tokens["typography.width"]
        })
        readonly property var axesTitle: ({
            "wght": root.tokens["typography.titleWeight"],
            "wdth": root.tokens["typography.width"]
        })

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
        readonly property list<real> expressiveFastSpatial: root.tokens["motion.curve.expressiveFastSpatial"]
        readonly property list<real> expressiveDefaultSpatial: root.tokens["motion.curve.expressiveDefaultSpatial"]
        readonly property list<real> expressiveSlowSpatial: root.tokens["motion.curve.expressiveSlowSpatial"]
        readonly property list<real> expressiveEffects: root.tokens["motion.curve.expressiveEffects"]
        readonly property list<real> emphasized: root.tokens["motion.curve.emphasized"]
        readonly property list<real> emphasizedAccel: root.tokens["motion.curve.emphasizedAccel"]
        readonly property list<real> emphasizedDecel: root.tokens["motion.curve.emphasizedDecel"]
        readonly property list<real> standard: root.tokens["motion.curve.standard"]
        readonly property list<real> standardDecel: root.tokens["motion.curve.standardDecel"]
    }

    readonly property QtObject duration: QtObject {
        readonly property int fast: root.tokens["motion.duration.fast"]
        readonly property int spatialFast: root.tokens["motion.duration.spatialFast"]
        readonly property int spatial: root.tokens["motion.duration.spatial"]
        readonly property int spatialSlow: root.tokens["motion.duration.spatialSlow"]
        readonly property int enter: root.tokens["motion.duration.enter"]
        readonly property int exit: root.tokens["motion.duration.exit"]
        readonly property int resize: root.tokens["motion.duration.resize"]
        readonly property int sheetEnter: root.tokens["motion.duration.sheetEnter"]
        readonly property int sheetExit: root.tokens["motion.duration.sheetExit"]
        readonly property int ripple: root.tokens["motion.duration.ripple"]
        readonly property int rippleFade: root.tokens["motion.duration.rippleFade"]
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
