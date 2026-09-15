#include "Value.h"

#include <algorithm>

namespace {

constexpr qsizetype kSymbolLimit = 64;

const QByteArray kUnrecognized = QByteArrayLiteral("unrecognized");
const QByteArray kNone = QByteArrayLiteral("none");

constexpr bool symbolic(char16_t character)
{
    return (character >= u'a' && character <= u'z') || (character >= u'A' && character <= u'Z')
        || (character >= u'0' && character <= u'9') || character == u'_' || character == u'.'
        || character == u'-' || character == u':' || character == u'/' || character == u'+';
}

template<typename Text> bool wellFormed(const Text &identifier)
{
    return !identifier.isEmpty() && identifier.size() <= kSymbolLimit
        && std::ranges::all_of(identifier,
                               [](auto character) { return symbolic(character.unicode()); });
}

template<typename Text> QByteArray checkedSymbol(const Text &identifier)
{
    if (identifier.isEmpty())
        return kNone;
    return wellFormed(identifier) ? identifier.toLatin1() : kUnrecognized;
}

}

namespace diagnostics {

Value Value::symbol(QStringView identifier)
{
    return Value(Checked {checkedSymbol(identifier)});
}

Value Value::symbol(QLatin1StringView identifier)
{
    return Value(Checked {checkedSymbol(QString(identifier))});
}

bool Value::symbolic(QLatin1StringView identifier)
{
    return wellFormed(QString(identifier));
}

QByteArray Value::text() const
{
    if (const auto *flag = std::get_if<bool>(&m_data))
        return *flag ? QByteArrayLiteral("true") : QByteArrayLiteral("false");
    if (const auto *integer = std::get_if<qint64>(&m_data))
        return QByteArray::number(*integer);
    if (const auto *real = std::get_if<double>(&m_data))
        return QByteArray::number(*real);
    return std::get<QByteArray>(m_data);
}

}
