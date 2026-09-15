#include "ModelWeights.h"

#include "core/Logging.h"

#include <QFile>
#include <QtEndian>

#include <algorithm>
#include <bit>

namespace {

constexpr char kMagic[] = "HTMBEAT";
constexpr int kMagicLength = 8;
constexpr quint32 kFormatVersion = 1;
constexpr int kMaximumRank = 4;
constexpr quint32 kMaximumExtent = 1u << 24u;

class Cursor
{
public:
    explicit Cursor(const QByteArray &blob)
        : m_blob(blob)
    {
    }

    bool has(qsizetype bytes) const { return m_position + bytes <= m_blob.size(); }
    qsizetype position() const { return m_position; }

    quint8 readByte()
    {
        const quint8 value = quint8(m_blob.at(m_position));
        m_position += 1;
        return value;
    }

    quint16 readShort()
    {
        const quint16 value = qFromLittleEndian<quint16>(m_blob.constData() + m_position);
        m_position += 2;
        return value;
    }

    quint32 readWord()
    {
        const quint32 value = qFromLittleEndian<quint32>(m_blob.constData() + m_position);
        m_position += 4;
        return value;
    }

    QByteArray readBytes(qsizetype length)
    {
        const QByteArray value = m_blob.mid(m_position, length);
        m_position += length;
        return value;
    }

private:
    const QByteArray &m_blob;
    qsizetype m_position = 0;
};

}

namespace analysis {

ModelWeights::ModelWeights(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(logTransition) << "beat model missing" << path;
        return;
    }
    m_valid = parse(file.readAll());
    if (!m_valid)
        qCWarning(logTransition) << "beat model unreadable" << path;
}

bool ModelWeights::parse(const QByteArray &blob)
{
    Cursor cursor(blob);
    if (!cursor.has(kMagicLength + 8))
        return false;
    if (cursor.readBytes(kMagicLength) != QByteArray(kMagic, kMagicLength))
        return false;
    if (cursor.readWord() != kFormatVersion)
        return false;

    const quint32 count = cursor.readWord();
    qsizetype total = 0;
    for (quint32 index = 0; index < count; ++index) {
        if (!cursor.has(3))
            return false;
        const quint16 length = cursor.readShort();
        if (!cursor.has(length + 1))
            return false;
        const QString name = QString::fromLatin1(cursor.readBytes(length));
        const quint8 rank = cursor.readByte();
        if (rank == 0 || rank > kMaximumRank || !cursor.has(qsizetype(rank) * 4))
            return false;

        Entry entry;
        entry.offset = total;
        entry.count = 1;
        for (quint8 dimension = 0; dimension < rank; ++dimension) {
            const quint32 extent = cursor.readWord();
            if (extent == 0 || extent > kMaximumExtent)
                return false;
            entry.shape.append(int(extent));
            entry.count *= extent;
        }
        total += entry.count;
        m_entries.insert(name, entry);
    }

    const qsizetype expected = total * qsizetype(sizeof(float));
    if (blob.size() - cursor.position() != expected)
        return false;

    m_values.resize(total);
    const char *source = blob.constData() + cursor.position();
    for (qsizetype index = 0; index < total; ++index)
        m_values[index] = std::bit_cast<float>(qFromLittleEndian<quint32>(source + index * 4));
    return true;
}

const float *ModelWeights::values(const QString &name, std::initializer_list<int> shape) const
{
    if (!m_valid)
        return nullptr;
    const auto entry = m_entries.constFind(name);
    if (entry == m_entries.constEnd()) {
        qCWarning(logTransition) << "beat model has no tensor" << name;
        return nullptr;
    }
    if (!std::equal(shape.begin(), shape.end(), entry->shape.cbegin(), entry->shape.cend())) {
        qCWarning(logTransition) << "beat model tensor" << name << "has shape" << entry->shape;
        return nullptr;
    }
    return m_values.constData() + entry->offset;
}

}
