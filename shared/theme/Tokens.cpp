#include "Tokens.h"

#include <QHash>
#include <QJsonArray>

namespace {

using theme::Token;

const QString kScalePrefix = QStringLiteral("shape.scale.");
const QString kSpeed = QStringLiteral("motion.speed");
constexpr qreal kRadiusLimit = 9999;
constexpr qreal kLengthLimit = 4000;
constexpr qreal kDurationLimit = 5000;
constexpr int kCurveSegment = 6;

Token step(const char *name, qreal value, const char *description)
{
    return {QString::fromLatin1(name),       Token::Step, value, 0, kRadiusLimit,
            QString::fromLatin1(description)};
}

Token radius(const char *name, const QVariant &value, const char *description)
{
    return {QString::fromLatin1(name),       Token::Radius, value, 0, kRadiusLimit,
            QString::fromLatin1(description)};
}

Token length(const char *name, int value, const char *follows, const char *description)
{
    return {QString::fromLatin1(name),
            Token::Length,
            value,
            0,
            kLengthLimit,
            QString::fromLatin1(description),
            QString::fromLatin1(follows)};
}

Token size(const char *name, int value, const char *description)
{
    return length(name, value, "density.size", description);
}

Token spacing(const char *name, int value, const char *description)
{
    return length(name, value, "density.spacing", description);
}

Token text(const char *name, int value, const char *description)
{
    return length(name, value, "density.text", description);
}

Token real(const char *name, qreal value, qreal minimum, qreal maximum, const char *description)
{
    return {QString::fromLatin1(name),       Token::Real, value, minimum, maximum,
            QString::fromLatin1(description)};
}

Token duration(const char *name, int value, const char *description)
{
    return {QString::fromLatin1(name),       Token::Duration, value, 0, kDurationLimit,
            QString::fromLatin1(description)};
}

Token curve(const char *name, const QVariantList &value, const char *description)
{
    return {QString::fromLatin1(name), Token::Curve, value, 0, 0, QString::fromLatin1(description)};
}

Token family(const char *name, const char *value, const char *description)
{
    return {QString::fromLatin1(name),       Token::Family, QString::fromLatin1(value), 0, 0,
            QString::fromLatin1(description)};
}

QVariant onStep(const char *name)
{
    return QString::fromLatin1(name);
}

std::optional<QVariant> curveFrom(const QJsonValue &value)
{
    const QJsonArray points = value.toArray();
    if (!value.isArray() || points.isEmpty() || points.size() % kCurveSegment != 0)
        return std::nullopt;

    QVariantList curve;
    for (const QJsonValue &point : points) {
        if (!point.isDouble())
            return std::nullopt;
        curve.append(point.toDouble());
    }

    if (curve.constLast().toDouble() != 1 || curve.at(curve.size() - 2).toDouble() != 1)
        return std::nullopt;
    return curve;
}

bool within(const Token &token, const QJsonValue &value)
{
    return value.isDouble() && value.toDouble() >= token.minimum
        && value.toDouble() <= token.maximum;
}

}

namespace theme::tokens {

const QList<Token> &all()
{
    static const QList<Token> kTokens = {
        step("shape.scale.extraSmall", 8, "The smallest step of the corner scale"),
        step("shape.scale.small", 12, "The small step of the corner scale"),
        step("shape.scale.medium", 17, "The medium step of the corner scale"),
        step("shape.scale.large", 23, "The large step of the corner scale"),
        step("shape.scale.extraLarge", 30, "The largest step of the corner scale"),
        step("shape.scale.full", 9999, "Fully rounded: a pill, or a circle on a square"),

        radius("shape.sheet", onStep("large"), "Modal sheets and confirmation prompts"),
        radius(
            "shape.popover", onStep("medium"),
            "Menus, popups, toasts and the tray menu. Rows inside them follow it, less the inset"),
        radius("shape.tooltip", onStep("extraSmall"), "Tooltips"),
        radius("shape.card", onStep("medium"),
               "Clickable cards: shelf cards, destinations, notifications, plugin rows"),
        radius("shape.row", onStep("small"), "Song rows, queue rows and picker rows"),
        radius("shape.field", onStep("small"), "Text fields, selects and the color field"),
        radius("shape.searchField", onStep("full"), "The search field"),
        radius("shape.playerBar", onStep("medium"), "The mini player"),
        radius("shape.fullPlayer", onStep("large"), "The full player"),
        radius("shape.artwork", onStep("small"), "Cover art on cards"),
        radius("shape.thumbnail", onStep("extraSmall"),
               "Small cover art in rows, the queue and the mini player"),
        radius("shape.cover", onStep("medium"),
               "Cover art in page headers and the top search result"),
        radius("shape.stage", onStep("large"),
               "The full player's artwork and video, and a podcast page's cover"),
        radius("shape.avatar", onStep("full"), "Artists, profiles and the account picture"),
        radius("shape.button", onStep("full"), "Text buttons"),
        radius("shape.iconButton", onStep("full"), "Icon buttons and the window buttons"),
        radius("shape.playButton", onStep("full"),
               "The play button in the players and on cover art"),
        radius("shape.chip", onStep("full"), "Filter chips, sort pills and chip selects"),
        radius("shape.badge", onStep("full"), "Live badges and small labels"),
        radius("shape.toggle", onStep("full"), "Switch tracks and thumbs"),
        radius("shape.segmented", onStep("full"), "Segmented controls and their indicator"),
        radius("shape.navigation", onStep("full"),
               "The selection indicator of the navigation rail and the settings sections"),
        radius("shape.track", onStep("full"),
               "Slider and progress tracks, their handles, scrollbars and tab indicators"),
        radius("shape.toolbar", onStep("full"), "The floating selection bar"),
        radius("shape.checkbox", 6, "Checkboxes"),
        radius("shape.swatch", onStep("full"), "Color swatches"),
        radius("shape.plate", onStep("full"),
               "Icon plates: Explore destinations, error states, the pinned badge"),

        real("density.spacing", 1, 0.5, 2,
             "Scales every gap, margin and padding. 0.75 is tighter, 1.25 roomier"),
        real("density.size", 1, 0.5, 2,
             "Scales the height and width of controls, rows, artwork and panels"),
        real("density.text", 1, 0.5, 2, "Scales every text size, and icons sized to match text"),

        size("size.row", 60, "Song rows in lists, albums and playlists"),
        size("size.card", 168, "The cover art of cards on shelves and in grids"),
        size("size.playerBar", 88, "The height of the mini player"),
        size("size.railCollapsed", 88, "The width of the navigation rail"),
        size("size.railExpanded", 232, "The width of the navigation rail when it shows labels"),
        size("size.contentMaxWidth", 1620,
             "The widest a page's content grows on a large window, its bleed included"),
        spacing("spacing.gutter", 24, "The space between the window's edge and its content"),
        spacing("spacing.bleed", 20,
                "Room at a page's edges for hovered cards to grow into. Keep it at least "
                "spacing.cardInset"),
        spacing("spacing.cardInset", 10,
                "How far a card's hover background reaches beyond its cover art"),

        text("typography.size.smallest", 10, "Tiny labels: badges, slider values, counters"),
        text("typography.size.smaller", 12, "Secondary text: artists, durations, captions"),
        text("typography.size.smallie", 13, "Labels on buttons, chips, tabs and the navigation"),
        text("typography.size.small", 15, "Titles in rows and cards, and text in fields"),
        text("typography.size.normal", 16, "Small icons in rows and buttons"),
        text("typography.size.large", 17, "Headings in empty states, destinations and sheets"),
        text("typography.size.larger", 19, "Most icons"),
        text("typography.size.huge", 22, "Sheet and section titles"),
        text("typography.size.display", 30, "The largest titles: page headers and the full player"),

        family("typography.family", "Roboto Flex",
               "Text font family. List a font file under fonts to use one that is not installed"),
        real("typography.weight", 450, 100, 1000,
             "Body text weight, on the wght axis of a variable font"),
        real("typography.titleWeight", 550, 100, 1000,
             "Title weight, on the wght axis of a variable font"),
        real("typography.width", 100, 25, 151, "Text width, on the wdth axis of a variable font"),
        family("icons.family", "Material Symbols Rounded",
               "Icon font family, drawn by ligature: Material Symbols Rounded, Sharp or Outlined"),

        real("state.hover", 0.08, 0, 1, "Opacity of the layer over a hovered control"),
        real("state.press", 0.10, 0, 1, "Opacity of the layer over a pressed control"),
        real("state.drag", 0.16, 0, 1, "Opacity of the layer over a dragged control"),
        real("state.ripple", 0.13, 0, 1, "Strength of the press ripple. 0 turns it off"),

        real("elevation.shadow", 1, 0, 2,
             "Strength of the shadow under floating surfaces. 0 removes it"),
        real("elevation.outline", 0, 0, 4,
             "Width of the outline around floating surfaces, in pixels"),

        real("motion.speed", 1, 0.25, 4, "Speed of every animation. 2 is twice as fast"),
        duration("motion.duration.fast", 200, "Color, opacity and state layer changes"),
        duration("motion.duration.spatialFast", 350, "Small movement and press bounces"),
        duration("motion.duration.spatial", 500, "Normal movement"),
        duration("motion.duration.spatialSlow", 650, "Large, slow movement"),
        duration("motion.duration.enter", 400, "Small elements entering"),
        duration("motion.duration.exit", 200, "Small elements leaving"),
        duration("motion.duration.resize", 300, "Panels growing and shrinking"),
        duration("motion.duration.sheetEnter", 500, "Sheets opening"),
        duration("motion.duration.sheetExit", 400, "Sheets closing"),
        duration("motion.duration.ripple", 1000, "The press ripple growing"),
        duration("motion.duration.rippleFade", 600, "The press ripple fading"),
        curve("motion.curve.expressiveFastSpatial", {0.42, 1.67, 0.21, 0.90, 1, 1},
              "Small movement, press bounce"),
        curve("motion.curve.expressiveDefaultSpatial", {0.38, 1.21, 0.22, 1.00, 1, 1},
              "Normal movement"),
        curve("motion.curve.expressiveSlowSpatial", {0.39, 1.29, 0.35, 0.98, 1, 1},
              "Large, slow movement"),
        curve("motion.curve.expressiveEffects", {0.34, 0.80, 0.34, 1.00, 1, 1},
              "Color, opacity, state layers"),
        curve("motion.curve.emphasized",
              {0.05, 0, 2.0 / 15, 0.06, 1.0 / 6, 0.4, 5.0 / 24, 0.82, 0.25, 1, 1, 1},
              "Sheets, panels, anything crossing the screen"),
        curve("motion.curve.emphasizedAccel", {0.3, 0, 0.8, 0.15, 1, 1}, "Small elements leaving"),
        curve("motion.curve.emphasizedDecel", {0.05, 0.7, 0.1, 1, 1, 1}, "Small elements entering"),
        curve("motion.curve.standard", {0.2, 0, 0, 1, 1, 1}, "Ordinary transitions"),
        curve("motion.curve.standardDecel", {0, 0, 0, 1, 1, 1}, "The press ripple"),
        curve("motion.curve.decelerate", {0.33, 1, 0.68, 1, 1, 1},
              "Progress filling up, a page fading in, a badge popping"),
    };
    return kTokens;
}

const Token *find(const QString &name)
{
    static const QHash<QString, qsizetype> kIndex = [] {
        QHash<QString, qsizetype> index;
        for (qsizetype i = 0; i < all().size(); ++i)
            index.insert(all().at(i).name, i);
        return index;
    }();

    const auto entry = kIndex.constFind(name);
    return entry == kIndex.constEnd() ? nullptr : &all().at(*entry);
}

const QStringList &steps()
{
    static const QStringList kSteps = [] {
        QStringList names;
        for (const Token &token : all()) {
            if (token.kind == Token::Step)
                names.append(token.name.mid(kScalePrefix.size()));
        }
        return names;
    }();
    return kSteps;
}

std::optional<QVariant> accept(const Token &token, const QJsonValue &value)
{
    switch (token.kind) {
    case Token::Step:
    case Token::Real:
        if (within(token, value))
            return value.toDouble();
        break;
    case Token::Duration:
    case Token::Length:
        if (within(token, value))
            return qRound(value.toDouble());
        break;
    case Token::Radius:
        if (value.isString() && steps().contains(value.toString()))
            return value.toString();
        if (within(token, value))
            return value.toDouble();
        break;
    case Token::Curve: return curveFrom(value);
    case Token::Family:
        if (value.isString() && !value.toString().trimmed().isEmpty())
            return value.toString().trimmed();
        break;
    }
    return std::nullopt;
}

QVariantMap resolve(const QVariantHash &chosen)
{
    const auto pick = [&chosen](const Token &token) {
        return chosen.value(token.name, token.fallback);
    };

    QHash<QString, qreal> scale;
    for (const Token &token : all()) {
        if (token.kind == Token::Step)
            scale.insert(token.name.mid(kScalePrefix.size()), pick(token).toDouble());
    }
    const qreal speed = pick(*find(kSpeed)).toDouble();

    QVariantMap values;
    for (const Token &token : all()) {
        const QVariant value = pick(token);
        switch (token.kind) {
        case Token::Radius:
            values.insert(token.name,
                          value.typeId() == QMetaType::QString ? scale.value(value.toString())
                                                               : value.toDouble());
            break;
        case Token::Duration: values.insert(token.name, qRound(value.toDouble() / speed)); break;
        case Token::Length:
            values.insert(token.name,
                          chosen.contains(token.name)
                              ? value.toInt()
                              : qRound(value.toDouble() * pick(*find(token.follows)).toDouble()));
            break;
        case Token::Step:
        case Token::Real:
        case Token::Curve:
        case Token::Family: values.insert(token.name, value); break;
        }
    }
    return values;
}

const QVariantMap &defaults()
{
    static const QVariantMap kDefaults = resolve({});
    return kDefaults;
}

}
