#include "LinkTarget.h"

#include <QRegularExpression>
#include <QStringList>
#include <QUrlQuery>

#include <algorithm>

namespace {

const QRegularExpression kVideoId(QStringLiteral("^[A-Za-z0-9_-]{11}$"));
const QRegularExpression kListId(QStringLiteral("^[A-Za-z0-9_-]+$"));
const QRegularExpression
    kPlaylistId(QStringLiteral("^(?:PL|OLAK5uy_|RD|UU|FL|PU)[A-Za-z0-9_-]{11,}$"));
const QRegularExpression kBrowseId(QStringLiteral("^(?:MPREb_|MPSP|MPED|VL)[A-Za-z0-9_-]{6,}$"));
const QRegularExpression kChannelId(QStringLiteral("^UC[A-Za-z0-9_-]{22}$"));
const QRegularExpression kHandle(QStringLiteral("^@[\\w.-]{3,}$"),
                                 QRegularExpression::UseUnicodePropertiesOption);
const QRegularExpression kWhitespace(QStringLiteral("\\s+"));

const QString kMusicHost = QStringLiteral("music.youtube.com");
const QString kShortHost = QStringLiteral("youtu.be");
const QString kEmbeddedPlaylist = QStringLiteral("videoseries");
const QStringList kDomains {QStringLiteral("youtube.com"), QStringLiteral("youtube-nocookie.com")};
const QStringList kVideoSections {QStringLiteral("shorts"), QStringLiteral("live"),
                                  QStringLiteral("embed"), QStringLiteral("v"),
                                  QStringLiteral("e")};

QUrl musicPage(const QString &path, const QString &list = {})
{
    QUrl url;
    url.setScheme(QStringLiteral("https"));
    url.setHost(kMusicHost);
    url.setPath(path);
    if (!list.isEmpty())
        url.setQuery(QUrlQuery {{QStringLiteral("list"), list}});
    return url;
}

bool onYouTube(const QString &host)
{
    return std::ranges::any_of(kDomains, [&host](const QString &domain) {
        return host == domain || host.endsWith(QLatin1Char('.') + domain);
    });
}

media::LinkTarget watching(const QString &videoId, const QString &playlistId)
{
    media::LinkTarget target;
    if (kVideoId.match(videoId).hasMatch() && videoId != kEmbeddedPlaylist)
        target.videoId = videoId;
    if (kListId.match(playlistId).hasMatch())
        target.playlistId = playlistId;
    return target;
}

media::LinkTarget fromId(const QString &id)
{
    if (kVideoId.match(id).hasMatch())
        return watching(id, {});
    media::LinkTarget target;
    if (kPlaylistId.match(id).hasMatch())
        target.page = musicPage(QStringLiteral("/playlist"), id);
    else if (kBrowseId.match(id).hasMatch())
        target.page = musicPage(QStringLiteral("/browse/") + id);
    else if (kChannelId.match(id).hasMatch())
        target.page = musicPage(QStringLiteral("/channel/") + id);
    else if (kHandle.match(id).hasMatch())
        target.page = musicPage(QLatin1Char('/') + id);
    return target;
}

media::LinkTarget fromUrl(const QUrl &url)
{
    const QString host = url.host().toLower();
    const QString path = url.path(QUrl::FullyDecoded);
    const QStringList sections = path.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    const QUrlQuery query(url);
    const QString list = query.queryItemValue(QStringLiteral("list"));
    if (host == kShortHost)
        return sections.isEmpty() ? media::LinkTarget() : watching(sections.first(), list);
    if (!onYouTube(host) || sections.isEmpty())
        return {};

    const QString &section = sections.first();
    if (section == QLatin1String("watch"))
        return watching(query.queryItemValue(QStringLiteral("v")), list);
    if (kVideoSections.contains(section) && sections.size() > 1)
        return watching(sections.at(1), list);

    media::LinkTarget target;
    if (section != QLatin1String("playlist"))
        target.page = musicPage(path);
    else if (kListId.match(list).hasMatch())
        target.page = musicPage(path, list);
    return target;
}

}

namespace media {

LinkTarget LinkTarget::parse(const QString &text)
{
    const QStringList tokens = text.split(kWhitespace, Qt::SkipEmptyParts);
    if (tokens.size() == 1) {
        const LinkTarget bare = fromId(tokens.first());
        if (bare.valid())
            return bare;
    }
    for (const QString &token : tokens) {
        const LinkTarget linked = fromUrl(QUrl::fromUserInput(token));
        if (linked.valid())
            return linked;
    }
    return {};
}

}
