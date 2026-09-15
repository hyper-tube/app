#include "SemanticVersion.h"

#include <algorithm>

namespace {

bool numeric(QStringView identifier)
{
    return !identifier.isEmpty()
        && std::ranges::all_of(identifier, [](QChar character) { return character.isDigit(); });
}

bool alphanumeric(QStringView identifier)
{
    return !identifier.isEmpty() && std::ranges::all_of(identifier, [](QChar character) {
        return (character.unicode() < 0x80 && character.isLetterOrNumber())
            || character == QLatin1Char('-');
    });
}

std::strong_ordering compareIdentifiers(const QString &left, const QString &right)
{
    const bool leftNumeric = numeric(left);
    const bool rightNumeric = numeric(right);
    if (leftNumeric && rightNumeric)
        return left.toULongLong() <=> right.toULongLong();
    if (leftNumeric != rightNumeric)
        return leftNumeric ? std::strong_ordering::less : std::strong_ordering::greater;
    return QString::compare(left, right, Qt::CaseSensitive) <=> 0;
}

}

namespace update {

SemanticVersion SemanticVersion::parse(QStringView text)
{
    SemanticVersion version;
    QStringView rest = text.trimmed();
    if (rest.startsWith(QLatin1Char('v')) || rest.startsWith(QLatin1Char('V')))
        rest = rest.sliced(1);

    const qsizetype build = rest.indexOf(QLatin1Char('+'));
    if (build >= 0)
        rest = rest.first(build);

    const qsizetype dash = rest.indexOf(QLatin1Char('-'));
    const QStringView core = dash >= 0 ? rest.first(dash) : rest;
    if (dash >= 0) {
        const QList<QStringView> identifiers = rest.sliced(dash + 1).split(QLatin1Char('.'));
        for (const QStringView identifier : identifiers) {
            if (!alphanumeric(identifier))
                return {};
            version.m_prerelease.append(identifier.toString());
        }
    }

    const QList<QStringView> parts = core.split(QLatin1Char('.'));
    if (parts.size() != qsizetype(version.m_core.size()))
        return {};
    for (qsizetype index = 0; index < parts.size(); ++index) {
        if (!numeric(parts.at(index)))
            return {};
        version.m_core.at(index) = parts.at(index).toULongLong();
    }

    version.m_valid = true;
    return version;
}

QString SemanticVersion::toString() const
{
    if (!m_valid)
        return {};
    QString text = QString::number(m_core.at(0)) + QLatin1Char('.') + QString::number(m_core.at(1))
        + QLatin1Char('.') + QString::number(m_core.at(2));
    if (prerelease())
        text += QLatin1Char('-') + m_prerelease.join(QLatin1Char('.'));
    return text;
}

std::strong_ordering SemanticVersion::operator<=>(const SemanticVersion &other) const
{
    if (m_valid != other.m_valid)
        return m_valid ? std::strong_ordering::greater : std::strong_ordering::less;
    if (const auto order = m_core <=> other.m_core; std::is_neq(order))
        return order;
    if (prerelease() != other.prerelease())
        return prerelease() ? std::strong_ordering::less : std::strong_ordering::greater;

    const qsizetype shared = std::min(m_prerelease.size(), other.m_prerelease.size());
    for (qsizetype index = 0; index < shared; ++index) {
        if (const auto order =
                compareIdentifiers(m_prerelease.at(index), other.m_prerelease.at(index));
            std::is_neq(order))
            return order;
    }
    return m_prerelease.size() <=> other.m_prerelease.size();
}

bool SemanticVersion::operator==(const SemanticVersion &other) const
{
    return std::is_eq(*this <=> other);
}

}
