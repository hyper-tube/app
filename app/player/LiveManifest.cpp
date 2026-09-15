#include "LiveManifest.h"

#include <QHash>
#include <QList>
#include <QRegularExpression>
#include <QString>
#include <QStringList>

namespace {

const QString kMediaTag = QStringLiteral("#EXT-X-MEDIA:");
const QString kVariantTag = QStringLiteral("#EXT-X-STREAM-INF:");

struct Variant
{
    QString uri;
    QString audioGroup;
    qint64 bandwidth = 0;
    int height = 0;
};

QHash<QString, QString> attributesOf(const QString &line)
{
    static const QRegularExpression attribute(QStringLiteral("([A-Z0-9-]+)=(\"[^\"]*\"|[^,]*)"));
    QHash<QString, QString> attributes;
    QRegularExpressionMatchIterator matches = attribute.globalMatch(line);
    while (matches.hasNext()) {
        const QRegularExpressionMatch match = matches.next();
        QString value = match.captured(2);
        if (value.size() >= 2 && value.startsWith(QLatin1Char('"')))
            value = value.mid(1, value.size() - 2);
        attributes.insert(match.captured(1), value);
    }
    return attributes;
}

QUrl resolve(const QUrl &base, const QString &uri)
{
    return base.resolved(QUrl::fromEncoded(uri.toUtf8()));
}

}

namespace player {

LiveManifest LiveManifest::parse(const QByteArray &playlist, const QUrl &base, int maximumHeight)
{
    QHash<QString, QString> audioGroups;
    QList<Variant> variants;
    const QStringList lines = QString::fromUtf8(playlist).split(QLatin1Char('\n'));
    for (qsizetype index = 0; index < lines.size(); ++index) {
        const QString line = lines.at(index).trimmed();
        if (line.startsWith(kMediaTag)) {
            const QHash<QString, QString> attributes = attributesOf(line.mid(kMediaTag.size()));
            if (attributes.value(QStringLiteral("TYPE")) == QLatin1String("AUDIO")
                && attributes.contains(QStringLiteral("URI")))
                audioGroups.insert(attributes.value(QStringLiteral("GROUP-ID")),
                                   attributes.value(QStringLiteral("URI")));
            continue;
        }
        if (!line.startsWith(kVariantTag) || index + 1 >= lines.size())
            continue;
        const QString uri = lines.at(index + 1).trimmed();
        if (uri.isEmpty() || uri.startsWith(QLatin1Char('#')))
            continue;
        const QHash<QString, QString> attributes = attributesOf(line.mid(kVariantTag.size()));
        variants.append(
            {uri, attributes.value(QStringLiteral("AUDIO")),
             attributes.value(QStringLiteral("BANDWIDTH")).toLongLong(),
             attributes.value(QStringLiteral("RESOLUTION")).section(QLatin1Char('x'), 1).toInt()});
    }

    const Variant *richest = nullptr;
    const Variant *leanest = nullptr;
    const Variant *picture = nullptr;
    for (const Variant &variant : std::as_const(variants)) {
        if (!richest || variant.bandwidth > richest->bandwidth)
            richest = &variant;
        if (!leanest || variant.bandwidth < leanest->bandwidth)
            leanest = &variant;
        if (variant.height <= 0 || variant.height > maximumHeight)
            continue;
        if (!picture || variant.height > picture->height
            || (variant.height == picture->height && variant.bandwidth > picture->bandwidth))
            picture = &variant;
    }

    LiveManifest manifest;
    if (richest && audioGroups.contains(richest->audioGroup))
        manifest.audio = resolve(base, audioGroups.value(richest->audioGroup));
    else if (leanest)
        manifest.audio = resolve(base, leanest->uri);
    if (picture) {
        manifest.picture = resolve(base, picture->uri);
        manifest.pictureHeight = picture->height;
    }
    return manifest;
}

}
