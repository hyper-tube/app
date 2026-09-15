#pragma once

#include <QByteArray>
#include <QString>
#include <QStringView>

#include <concepts>
#include <initializer_list>
#include <utility>
#include <variant>

namespace diagnostics {

enum class Level {
    Debug,
    Info,
    Warning,
    Error,
    Fatal,
};

class Value
{
public:
    using Data = std::variant<bool, qint64, double, QByteArray>;

    explicit Value(bool value)
        : m_data(value)
    {
    }

    template<std::integral Integer>
        requires(!std::same_as<Integer, bool>)
    explicit Value(Integer value)
        : m_data(qint64(value))
    {
    }

    template<std::floating_point Real>
    explicit Value(Real value)
        : m_data(double(value))
    {
    }

    explicit Value(const char *literal)
        : m_data(QByteArray(literal))
    {
    }

    explicit Value(const QString &) = delete;
    explicit Value(QStringView) = delete;
    explicit Value(const QByteArray &) = delete;

    static Value symbol(QStringView identifier);
    static Value symbol(QLatin1StringView identifier);
    static bool symbolic(QLatin1StringView identifier);

    const Data &data() const { return m_data; }
    QByteArray text() const;

private:
    struct Checked
    {
        QByteArray text;
    };

    explicit Value(Checked checked)
        : m_data(std::move(checked.text))
    {
    }

    Data m_data;
};

struct Field
{
    template<typename Content>
    Field(const char *name, Content content)
        : key(name)
        , value(std::move(content))
    {
    }

    const char *key;
    Value value;
};

using Fields = std::initializer_list<Field>;

}
