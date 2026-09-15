#pragma once

#include <QHash>
#include <QList>
#include <QString>

#include <initializer_list>

namespace analysis {

class ModelWeights
{
public:
    explicit ModelWeights(const QString &path);

    bool valid() const { return m_valid; }
    const float *values(const QString &name, std::initializer_list<int> shape) const;

private:
    struct Entry
    {
        QList<int> shape;
        qsizetype offset = 0;
        qsizetype count = 0;
    };

    bool parse(const QByteArray &blob);

    QHash<QString, Entry> m_entries;
    QList<float> m_values;
    bool m_valid = false;
};

}
