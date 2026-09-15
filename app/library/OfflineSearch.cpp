#include "OfflineSearch.h"

#include "Downloads.h"

#include <QSet>

#include <algorithm>
#include <optional>

namespace {

enum Rank {
    TitleStart,
    TitleMatch,
    ArtistMatch,
    WordMatch,
};

struct Candidate
{
    media::Track track;
    Rank rank = WordMatch;
};

QString folded(const QString &text)
{
    return text.simplified().toCaseFolded();
}

std::optional<Rank> rankOf(const media::Track &track, const QString &needle,
                           const QStringList &words)
{
    const QString title = folded(track.title);
    const QString artist = folded(track.artist);
    const QString haystack =
        title + QLatin1Char(' ') + artist + QLatin1Char(' ') + folded(track.album);
    for (const QString &word : words) {
        if (!haystack.contains(word))
            return std::nullopt;
    }
    if (title.startsWith(needle))
        return TitleStart;
    if (title.contains(needle))
        return TitleMatch;
    if (artist.contains(needle))
        return ArtistMatch;
    return WordMatch;
}

}

namespace library::offlineSearch {

QList<media::Track> matches(const QString &query)
{
    const QString needle = folded(query);
    const QStringList words = needle.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (words.isEmpty())
        return {};

    QList<Candidate> candidates;
    for (const media::Track &track : Downloads::instance().tracks()) {
        if (const std::optional<Rank> rank = rankOf(track, needle, words))
            candidates.append({track, *rank});
    }
    std::ranges::stable_sort(candidates, {}, &Candidate::rank);

    QList<media::Track> found;
    found.reserve(candidates.size());
    for (const Candidate &candidate : std::as_const(candidates))
        found.append(candidate.track);
    return found;
}

QStringList suggestions(const QString &query, int limit)
{
    const QString needle = folded(query);
    QStringList offered;
    QSet<QString> seen;
    const auto offer = [&offered, &seen, &needle, limit](const QString &text) {
        const QString key = folded(text);
        if (offered.size() >= limit || key.isEmpty() || !key.contains(needle) || seen.contains(key))
            return;
        seen.insert(key);
        offered.append(text);
    };
    for (const media::Track &track : matches(query)) {
        offer(track.title);
        offer(track.artist);
    }
    return offered;
}

}
