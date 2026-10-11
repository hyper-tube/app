#include "ThemeFile.h"

#include "Palette.h"
#include "Tokens.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>

namespace {

constexpr int kFormat = 1;

const QString kSchemaKey = QStringLiteral("$schema");
const QString kFormatKey = QStringLiteral("schema");
const QString kNameKey = QStringLiteral("name");
const QString kDescriptionKey = QStringLiteral("description");
const QString kAuthorKey = QStringLiteral("author");
const QString kVersionKey = QStringLiteral("version");
const QString kExtendsKey = QStringLiteral("extends");
const QString kFontsKey = QStringLiteral("fonts");
const QString kColorsKey = QStringLiteral("colors");
const QString kSeedKey = QStringLiteral("seed");
const QString kLightKey = QStringLiteral("light");
const QString kDarkKey = QStringLiteral("dark");
const QString kOverridesKey = QStringLiteral("overrides");
const QString kSchemesKey = QStringLiteral("schemes");

const QStringList kMaterialThemeBuilderKeys = {
    QStringLiteral("description"),
    QStringLiteral("coreColors"),
    QStringLiteral("extendedColors"),
    QStringLiteral("palettes"),
};

QString snakeCase(const QString &name)
{
    QString result;
    for (const QChar character : name) {
        if (character.isUpper()) {
            result.append(QLatin1Char('_'));
            result.append(character.toLower());
        } else {
            result.append(character);
        }
    }
    return result;
}

QString member(const QString &where, const QString &key)
{
    return where + QLatin1Char('.') + key;
}

}

namespace theme {

ThemeFile ThemeFile::read(const QString &id, const QString &path)
{
    ThemeFile file;
    file.id = id;
    file.path = path;
    file.name = id;

    QFile source(path);
    if (!source.open(QIODevice::ReadOnly)) {
        file.problems.append(tr("The theme file could not be read"));
        return file;
    }

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(source.readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        file.problems.append(tr("The theme file is not valid JSON: %1, at character %2")
                                 .arg(error.errorString())
                                 .arg(error.offset));
        return file;
    }
    if (!document.isObject()) {
        file.problems.append(tr("The theme file has to hold a JSON object"));
        return file;
    }

    file.take(document.object());
    return file;
}

int ThemeFile::format()
{
    return kFormat;
}

QString ThemeFile::directory() const
{
    return QFileInfo(path).absolutePath();
}

QString ThemeFile::expectation(const Token &token)
{
    switch (token.kind) {
    case Token::Step:
    case Token::Real: return tr("a number from %1 to %2").arg(token.minimum).arg(token.maximum);
    case Token::Radius:
        return tr("a number from %1 to %2, or one of %3")
            .arg(token.minimum)
            .arg(token.maximum)
            .arg(tokens::steps().join(QStringLiteral(", ")));
    case Token::Duration:
        return tr("a number of milliseconds from %1 to %2").arg(token.minimum).arg(token.maximum);
    case Token::Curve:
        return tr("a list of Bezier control points in groups of six, ending in 1, 1");
    case Token::Family: return tr("the name of a font family");
    }
    return {};
}

void ThemeFile::take(const QJsonObject &document)
{
    for (auto entry = document.constBegin(); entry != document.constEnd(); ++entry) {
        const QString &key = entry.key();
        const QJsonValue value = entry.value();

        if (key == kSchemaKey || key == kVersionKey)
            continue;
        if (key == kFormatKey) {
            if (value.toInt(kFormat) > kFormat)
                problems.append(tr("The theme was written for a newer version of the app, so "
                                   "values this version does not know are skipped"));
        } else if (key == kNameKey) {
            if (const QString text = textFrom(key, value); !text.isEmpty())
                name = text;
        } else if (key == kDescriptionKey) {
            description = textFrom(key, value);
        } else if (key == kAuthorKey) {
            author = textFrom(key, value);
        } else if (key == kExtendsKey) {
            extends = textFrom(key, value);
        } else if (key == kFontsKey) {
            takeFonts(value);
        } else if (key == kColorsKey) {
            takeColors(value);
        } else {
            collect(key, value);
        }
    }
}

void ThemeFile::collect(const QString &name, const QJsonValue &value)
{
    if (const Token *token = tokens::find(name)) {
        if (const std::optional<QVariant> accepted = tokens::accept(*token, value))
            values.insert(name, *accepted);
        else
            problems.append(tr("%1 should be %2").arg(name, expectation(*token)));
        return;
    }

    if (value.isObject()) {
        const QJsonObject object = value.toObject();
        for (auto entry = object.constBegin(); entry != object.constEnd(); ++entry)
            collect(member(name, entry.key()), entry.value());
        return;
    }

    problems.append(tr("%1 is not something a theme can set").arg(name));
}

void ThemeFile::takeColors(const QJsonValue &value)
{
    if (!value.isObject()) {
        problems.append(tr("%1 should be an object").arg(kColorsKey));
        return;
    }

    const QJsonObject colors = value.toObject();
    for (auto entry = colors.constBegin(); entry != colors.constEnd(); ++entry) {
        const QString &key = entry.key();
        const QString where = member(kColorsKey, key);

        if (key == kSeedKey) {
            const QColor color = QColor::fromString(entry.value().toString());
            if (entry.value().isString() && color.isValid())
                seed = color;
            else
                problems.append(tr("%1 is not a color").arg(where));
        } else if (key == kLightKey) {
            light.insert(rolesFrom(where, entry.value()));
        } else if (key == kDarkKey) {
            dark.insert(rolesFrom(where, entry.value()));
        } else if (key == kOverridesKey) {
            takeOverrides(entry.value());
        } else if (key == kSchemesKey) {
            takeSchemes(entry.value());
        } else if (!kMaterialThemeBuilderKeys.contains(key)) {
            problems.append(tr("%1 is not something a theme can set").arg(where));
        }
    }
}

void ThemeFile::takeOverrides(const QJsonValue &value)
{
    const QString where = member(kColorsKey, kOverridesKey);
    if (!value.isObject()) {
        problems.append(tr("%1 should be an object").arg(where));
        return;
    }

    const QJsonObject overrides = value.toObject();
    for (auto entry = overrides.constBegin(); entry != overrides.constEnd(); ++entry) {
        if (entry.key() == kLightKey)
            lightOverrides.insert(rolesFrom(member(where, entry.key()), entry.value()));
        else if (entry.key() == kDarkKey)
            darkOverrides.insert(rolesFrom(member(where, entry.key()), entry.value()));
        else
            problems.append(
                tr("%1 is not something a theme can set").arg(member(where, entry.key())));
    }
}

void ThemeFile::takeSchemes(const QJsonValue &value)
{
    const QString where = member(kColorsKey, kSchemesKey);
    if (!value.isObject()) {
        problems.append(tr("%1 should be an object").arg(where));
        return;
    }

    const QJsonObject schemes = value.toObject();
    if (schemes.contains(kLightKey))
        light.insert(rolesFrom(member(where, kLightKey), schemes.value(kLightKey)));
    if (schemes.contains(kDarkKey))
        dark.insert(rolesFrom(member(where, kDarkKey), schemes.value(kDarkKey)));
}

void ThemeFile::takeFonts(const QJsonValue &value)
{
    if (!value.isArray()) {
        problems.append(tr("%1 should be a list of font files").arg(kFontsKey));
        return;
    }

    for (const QJsonValue &font : value.toArray()) {
        if (font.isString() && !font.toString().isEmpty())
            fonts.append(font.toString());
        else
            problems.append(tr("%1 should be a list of font files").arg(kFontsKey));
    }
}

ThemeScheme::Roles ThemeFile::rolesFrom(const QString &where, const QJsonValue &value)
{
    static const QStringList kRoles = Palette::roleNames();

    ThemeScheme::Roles roles;
    if (!value.isObject()) {
        problems.append(tr("%1 should be an object of color roles").arg(where));
        return roles;
    }

    const QJsonObject object = value.toObject();
    for (auto entry = object.constBegin(); entry != object.constEnd(); ++entry) {
        const QString role = snakeCase(entry.key());
        const QString name = member(where, entry.key());
        if (!kRoles.contains(role)) {
            problems.append(tr("%1 is not a color role").arg(name));
            continue;
        }

        const QColor color = QColor::fromString(entry.value().toString());
        if (!entry.value().isString() || !color.isValid()) {
            problems.append(tr("%1 is not a color").arg(name));
            continue;
        }
        roles.insert(role, color);
    }
    return roles;
}

QString ThemeFile::textFrom(const QString &where, const QJsonValue &value)
{
    if (value.isString())
        return value.toString().trimmed();
    problems.append(tr("%1 should be text").arg(where));
    return {};
}

}
