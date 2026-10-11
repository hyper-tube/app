#include "ThemeSchema.h"

#include "Palette.h"
#include "ThemeFile.h"
#include "Tokens.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

using theme::Token;

const QString kProperties = QStringLiteral("properties");
const QString kType = QStringLiteral("type");
const QString kDescription = QStringLiteral("description");
const QString kDefault = QStringLiteral("default");
const QString kMinimum = QStringLiteral("minimum");
const QString kMaximum = QStringLiteral("maximum");
const QString kAdditional = QStringLiteral("additionalProperties");

QJsonObject text(const QString &description)
{
    return {{kType, QStringLiteral("string")}, {kDescription, description}};
}

QJsonObject number(const Token &token, const QString &type)
{
    return {{kType, type},
            {kMinimum, token.minimum},
            {kMaximum, token.maximum},
            {kDescription, token.description},
            {kDefault, QJsonValue::fromVariant(token.fallback)}};
}

QJsonObject propertyOf(const Token &token)
{
    switch (token.kind) {
    case Token::Step:
    case Token::Real: return number(token, QStringLiteral("number"));
    case Token::Duration: return number(token, QStringLiteral("integer"));
    case Token::Radius:
        return {{QStringLiteral("anyOf"),
                 QJsonArray {QJsonObject {{kType, QStringLiteral("number")},
                                          {kMinimum, token.minimum},
                                          {kMaximum, token.maximum}},
                             QJsonObject {{QStringLiteral("enum"),
                                           QJsonArray::fromStringList(theme::tokens::steps())}}}},
                {kDescription, token.description},
                {kDefault, QJsonValue::fromVariant(token.fallback)}};
    case Token::Curve:
        return {{kType, QStringLiteral("array")},
                {QStringLiteral("items"), QJsonObject {{kType, QStringLiteral("number")}}},
                {QStringLiteral("minItems"), 6},
                {kDescription, token.description},
                {kDefault, QJsonValue::fromVariant(token.fallback)}};
    case Token::Family:
        return {{kType, QStringLiteral("string")},
                {kDescription, token.description},
                {kDefault, QJsonValue::fromVariant(token.fallback)}};
    }
    return {};
}

QJsonObject group()
{
    return {{kType, QStringLiteral("object")}, {kAdditional, false}};
}

void place(QJsonObject &node, QStringList path, const QJsonObject &leaf)
{
    QJsonObject properties = node.value(kProperties).toObject();
    const QString key = path.takeFirst();
    if (path.isEmpty()) {
        properties.insert(key, leaf);
    } else {
        QJsonObject child = properties.contains(key) ? properties.value(key).toObject() : group();
        place(child, path, leaf);
        properties.insert(key, child);
    }
    node.insert(kProperties, properties);
}

QJsonObject roles(const QString &description)
{
    QJsonObject properties;
    for (const QString &role : theme::Palette::roleNames())
        properties.insert(role, text(QStringLiteral("A color, such as #6750A4")));
    return {{kType, QStringLiteral("object")},
            {kDescription, description},
            {kProperties, properties},
            {kAdditional, QJsonObject {{kType, QStringLiteral("string")}}}};
}

QJsonObject colors()
{
    const QJsonObject paired {
        {kType, QStringLiteral("object")},
        {kProperties,
         QJsonObject {{QStringLiteral("light"), roles(QStringLiteral("Roles used in light mode"))},
                      {QStringLiteral("dark"), roles(QStringLiteral("Roles used in dark mode"))}}}};

    QJsonObject overrides = paired;
    overrides.insert(kDescription,
                     QStringLiteral("Roles laid over whichever palette is in use, including the "
                                    "desktop's and the user's own accent color"));
    overrides.insert(kAdditional, false);

    QJsonObject schemes = paired;
    schemes.insert(kDescription,
                   QStringLiteral("The schemes of a Material Theme Builder export, used as light "
                                  "and dark"));

    return {
        {kType, QStringLiteral("object")},
        {kDescription,
         QStringLiteral("The theme's own palette. It is offered beside the desktop's color "
                        "sources and picked when the theme is applied. Material roles in "
                        "snake_case or camelCase; roles left out are generated from the seed")},
        {kProperties,
         QJsonObject {
             {QStringLiteral("seed"),
              text(QStringLiteral("The color every role left out is generated from"))},
             {QStringLiteral("light"), roles(QStringLiteral("Roles used in light mode"))},
             {QStringLiteral("dark"), roles(QStringLiteral("Roles used in dark mode"))},
             {QStringLiteral("overrides"), overrides},
             {QStringLiteral("schemes"), schemes},
         }},
    };
}

}

namespace theme {

QByteArray themeSchema()
{
    QJsonObject schema {
        {QStringLiteral("$schema"), QStringLiteral("http://json-schema.org/draft-07/schema#")},
        {QStringLiteral("title"), QStringLiteral("HyperTube Music theme")},
        {kType, QStringLiteral("object")},
        {kAdditional, false},
        {kProperties,
         QJsonObject {
             {QStringLiteral("$schema"), text(QStringLiteral("Where this schema lives"))},
             {QStringLiteral("schema"),
              QJsonObject {{QStringLiteral("const"), ThemeFile::format()},
                           {kDescription, QStringLiteral("The version of the theme format")}}},
             {QStringLiteral("name"), text(QStringLiteral("The name shown in Settings"))},
             {QStringLiteral("description"), text(QStringLiteral("One line shown under the name"))},
             {QStringLiteral("author"), text(QStringLiteral("Who made the theme"))},
             {QStringLiteral("version"), text(QStringLiteral("The theme's own version"))},
             {QStringLiteral("extends"),
              text(QStringLiteral("The id of the theme this one starts from: default, square, "
                                  "or the folder name of another theme"))},
             {QStringLiteral("fonts"),
              QJsonObject {
                  {kType, QStringLiteral("array")},
                  {QStringLiteral("items"), QJsonObject {{kType, QStringLiteral("string")}}},
                  {kDescription,
                   QStringLiteral("Font files inside the theme folder to load, for "
                                  "typography.family and icons.family")}}},
             {QStringLiteral("colors"), colors()},
         }},
    };

    for (const Token &token : tokens::all())
        place(schema, token.name.split(QLatin1Char('.')), propertyOf(token));

    return QJsonDocument(schema).toJson(QJsonDocument::Indented);
}

}
