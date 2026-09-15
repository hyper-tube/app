#include "DownloadPlanner.h"

#include <QDateTime>

#include <algorithm>
#include <cmath>

namespace {

constexpr double kDecaySeconds = 21.0 * 24 * 60 * 60;
constexpr double kLikeBonus = 2.0;
constexpr qint64 kEstimatedBytesPerSecond = 24000;

}

namespace library {

QList<media::Track> DownloadPlanner::rank(const QHash<QString, PlayLog::Entry> &entries)
{
    struct Candidate
    {
        media::Track track;
        double score;
    };

    QList<Candidate> candidates;
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    for (const PlayLog::Entry &entry : entries) {
        if (entry.track.episode)
            continue;
        const double recency = std::exp(-qMax(qint64(0), now - entry.lastPlayed) / kDecaySeconds);
        const double score = qMax(0, entry.plays - entry.skips) * recency * entry.completion
            + (entry.track.liked ? kLikeBonus : 0.0);
        if (score > 0)
            candidates.append({entry.track, score});
    }
    std::ranges::sort(candidates, [](const Candidate &left, const Candidate &right) {
        return left.score == right.score ? left.track.videoId < right.track.videoId
                                         : left.score > right.score;
    });
    QList<media::Track> ranked;
    for (const Candidate &candidate : candidates)
        ranked.append(candidate.track);
    return ranked;
}

QSet<QString> DownloadPlanner::choose(const QList<media::Track> &ranked, int count, qint64 budget,
                                      const QHash<QString, qint64> &sizes,
                                      const QSet<QString> &forced)
{
    QSet<QString> selected;
    for (const media::Track &track : ranked) {
        if (selected.size() >= count)
            break;
        if (forced.contains(track.videoId))
            continue;
        const qint64 estimate = sizes.value(
            track.videoId, qMax(qint64(60), track.durationMs / 1000) * kEstimatedBytesPerSecond);
        if (estimate > budget)
            continue;
        budget -= estimate;
        selected.insert(track.videoId);
    }
    return selected;
}

}
