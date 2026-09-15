#pragma once

#include "AnalysisStore.h"
#include "player/StreamResolver.h"

#include <QHash>
#include <QObject>
#include <QThreadPool>

#include <atomic>
#include <memory>
#include <optional>

namespace analysis {

class Analyzer : public QObject
{
    Q_OBJECT

public:
    enum class Depth {
        Loudness,
        Full,
    };

    struct Outcome
    {
        std::optional<TrackAnalysis> analysis;
        qint64 elapsedMs = 0;
        qint64 cpuMs = 0;
        qint64 beatCpuMs = 0;
        qint64 frontendCpuMs = 0;
        bool modelAvailable = false;
    };

    explicit Analyzer(QObject *parent = nullptr);
    ~Analyzer() override;

    void setTargets(const QStringList &videoIds);
    void request(const player::Stream &stream, Depth depth);
    std::optional<TrackAnalysis> analysis(const QString &videoId, int itag) const;
    std::optional<TrackAnalysis> analysisFor(const QString &videoId) const;

Q_SIGNALS:
    void ready(const analysis::TrackAnalysis &analysis);

private:
    struct Job
    {
        QString videoId;
        Depth depth = Depth::Full;
        std::shared_ptr<std::atomic_bool> cancelled;
    };

    static QString keyFor(const QString &videoId, int itag);
    static bool satisfies(const TrackAnalysis &analysis, Depth depth);
    void complete(const QString &key, const std::shared_ptr<std::atomic_bool> &cancelled,
                  const Outcome &outcome);

    AnalysisStore m_store;
    QThreadPool m_pool;
    QHash<QString, TrackAnalysis> m_analyses;
    QHash<QString, Job> m_jobs;
    QSet<QString> m_targets;
};

}
