#pragma once

#include <QString>
#include <QStringList>
#include <QStringView>

#include <array>
#include <compare>

namespace update {

class SemanticVersion
{
public:
    static SemanticVersion parse(QStringView text);

    bool valid() const { return m_valid; }
    bool prerelease() const { return !m_prerelease.isEmpty(); }
    QString toString() const;

    std::strong_ordering operator<=>(const SemanticVersion &other) const;
    bool operator==(const SemanticVersion &other) const;

private:
    std::array<quint64, 3> m_core {};
    QStringList m_prerelease;
    bool m_valid = false;
};

}
