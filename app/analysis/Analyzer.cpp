#include "Analyzer.h"

#include "BeatTracker.h"
#include "Loudness.h"
#include "PcmReader.h"
#include "core/Logging.h"

#include <QElapsedTimer>
#include <QMetaObject>
#include <QRunnable>
#include <QThread>

#include <ctime>
#include <functional>

namespace {

class AnalysisJob final : public QRunnable
{
public:
    using Completion = std::function<void(const analysis::Analyzer::Outcome &)>;

    AnalysisJob(player::Stream stream, analysis::Analyzer::Depth depth,
                std::shared_ptr<std::atomic_bool> cancelled, Completion completion)
        : m_stream(std::move(stream))
        , m_depth(depth)
        , m_cancelled(std::move(cancelled))
        , m_completion(std::move(completion))
    {
        setAutoDelete(true);
    }

    void run() override
    {
        QElapsedTimer elapsed;
        elapsed.start();
        const std::clock_t cpuStart = std::clock();
        const std::optional<analysis::PcmReader::Result> pcm =
            analysis::PcmReader::read(m_stream, *m_cancelled);
        analysis::Analyzer::Outcome outcome;
        if (pcm && !m_cancelled->load()) {
            const analysis::Loudness::Result loudness =
                analysis::Loudness::measure(pcm->samples, analysis::PcmReader::kSampleRate);
            analysis::TrackAnalysis analysis;
            analysis.videoId = m_stream.videoId;
            analysis.itag = m_stream.itag;
            analysis.durationMs = pcm->durationMs;
            analysis.leadingSilenceEndMs = loudness.leadingSilenceEndMs;
            analysis.trailingSilenceStartMs = loudness.trailingSilenceStartMs;
            analysis.fadeOutStartMs = loudness.fadeOutStartMs;
            analysis.integratedLufs = loudness.integratedLufs;
            analysis.shortTermLufs = loudness.shortTermLufs;

            if (m_depth == analysis::Analyzer::Depth::Full) {
                const analysis::BeatTracker::Result beats = analysis::BeatTracker::track(
                    pcm->samples, analysis::PcmReader::kSampleRate, *m_cancelled);
                analysis.beatsAttempted = true;
                analysis.beatsAvailable = beats.modelAvailable && !beats.regions.isEmpty();
                analysis.beatRegions = beats.regions;
                analysis.structures = beats.structures;
                analysis.keys = beats.keys;
                outcome.modelAvailable = beats.modelAvailable;
                outcome.beatCpuMs = beats.cpuMs;
                outcome.frontendCpuMs = beats.frontendCpuMs;
            }
            if (analysis.valid())
                outcome.analysis = std::move(analysis);
        }
        outcome.elapsedMs = elapsed.elapsed();
        outcome.cpuMs = qint64(1000.0 * double(std::clock() - cpuStart) / CLOCKS_PER_SEC);
        m_completion(outcome);
    }

private:
    player::Stream m_stream;
    analysis::Analyzer::Depth m_depth;
    std::shared_ptr<std::atomic_bool> m_cancelled;
    Completion m_completion;
};

void logRegions(const analysis::TrackAnalysis &analysis)
{
    for (int index = 0; index < analysis.beatRegions.size(); ++index) {
        const analysis::BeatRegion &region = analysis.beatRegions.at(index);
        qCInfo(logTransition) << "beat region" << analysis.videoId << "start_ms" << region.startMs
                              << "end_ms" << region.endMs << "beats" << region.beatsMs.size()
                              << "downbeats" << region.downbeatsMs.size() << "tempo" << region.tempo
                              << "phase_ms" << region.phaseMs << "beat_confidence"
                              << region.beatConfidence << "downbeat_confidence"
                              << region.downbeatConfidence;
        if (index >= analysis.structures.size() || index >= analysis.keys.size())
            continue;
        const analysis::Structure::Result &structure = analysis.structures.at(index);
        const analysis::KeyEstimator::Result &key = analysis.keys.at(index);
        qCInfo(logTransition) << "structure" << analysis.videoId << "start_ms" << region.startMs
                              << "bars" << structure.barsMs.size() << "bar_ms" << structure.barMs
                              << "beats_per_bar" << structure.beatsPerBar << "phrases"
                              << structure.phrasesMs.size() << "intro_end_ms"
                              << structure.introEndMs << "outro_start_ms" << structure.outroStartMs
                              << "musical_end_ms" << structure.musicalEndMs << "bass_share"
                              << structure.bassShare << "key" << qUtf8Printable(key.name())
                              << "key_confidence" << key.confidence;
    }
}

}

namespace analysis {

Analyzer::Analyzer(QObject *parent)
    : QObject(parent)
{
    m_pool.setMaxThreadCount(1);
    m_pool.setThreadPriority(QThread::LowestPriority);
}

Analyzer::~Analyzer()
{
    for (const Job &job : m_jobs)
        job.cancelled->store(true);
    m_pool.clear();
    m_pool.waitForDone();
}

QString Analyzer::keyFor(const QString &videoId, int itag)
{
    return videoId + QLatin1Char('-') + QString::number(itag);
}

void Analyzer::setTargets(const QStringList &videoIds)
{
    m_targets = QSet<QString>(videoIds.cbegin(), videoIds.cend());
    for (const Job &job : m_jobs) {
        if (!m_targets.contains(job.videoId))
            job.cancelled->store(true);
    }
}

void Analyzer::request(const player::Stream &stream, Depth depth)
{
    if (!stream.valid() || stream.videoId.isEmpty() || !m_targets.contains(stream.videoId))
        return;

    const QString key = keyFor(stream.videoId, stream.itag);
    const auto held = m_analyses.constFind(key);
    if (held != m_analyses.constEnd() && satisfies(*held, depth))
        return;
    const auto running = m_jobs.find(key);
    if (running != m_jobs.end()) {
        if (!running->cancelled->load() && running->depth >= depth)
            return;
        running->cancelled->store(true);
        m_jobs.erase(running);
    }

    if (held == m_analyses.constEnd()) {
        if (const std::optional<TrackAnalysis> stored = m_store.load(stream.videoId, stream.itag)) {
            m_analyses.insert(key, *stored);
            qCInfo(logTransition) << "analysis cache" << stream.videoId << "itag" << stream.itag
                                  << "beats" << stored->beatsAvailable;
            logRegions(*stored);
            Q_EMIT ready(*stored);
            if (satisfies(*stored, depth))
                return;
        }
    }

    const auto cancelled = std::make_shared<std::atomic_bool>(false);
    m_jobs.insert(key, {stream.videoId, depth, cancelled});
    m_pool.start(
        new AnalysisJob(stream, depth, cancelled, [this, key, cancelled](const Outcome &outcome) {
        QMetaObject::invokeMethod(this, [this, key, cancelled, outcome] {
            complete(key, cancelled, outcome);
        }, Qt::QueuedConnection);
    }));
}

bool Analyzer::satisfies(const TrackAnalysis &analysis, Depth depth)
{
    return depth == Depth::Loudness || analysis.beatsAttempted;
}

std::optional<TrackAnalysis> Analyzer::analysis(const QString &videoId, int itag) const
{
    const auto result = m_analyses.constFind(keyFor(videoId, itag));
    if (result == m_analyses.constEnd())
        return std::nullopt;
    return *result;
}

std::optional<TrackAnalysis> Analyzer::analysisFor(const QString &videoId) const
{
    if (videoId.isEmpty())
        return std::nullopt;
    for (auto entry = m_analyses.constBegin(); entry != m_analyses.constEnd(); ++entry) {
        if (entry->videoId == videoId)
            return *entry;
    }
    return std::nullopt;
}

void Analyzer::complete(const QString &key, const std::shared_ptr<std::atomic_bool> &cancelled,
                        const Outcome &outcome)
{
    const auto job = m_jobs.find(key);
    if (job == m_jobs.end() || job->cancelled != cancelled)
        return;
    m_jobs.erase(job);
    if (cancelled->load() || !outcome.analysis)
        return;

    const TrackAnalysis &analysis = *outcome.analysis;
    m_analyses.insert(key, analysis);
    if (!m_store.save(analysis))
        qCWarning(logTransition) << "cannot store analysis" << analysis.videoId << "itag"
                                 << analysis.itag;
    qCInfo(logTransition) << "analysis" << analysis.videoId << "itag" << analysis.itag << "wall_ms"
                          << outcome.elapsedMs << "cpu_ms" << outcome.cpuMs << "beat_cpu_ms"
                          << outcome.beatCpuMs << "frontend_cpu_ms" << outcome.frontendCpuMs
                          << "beat_model"
                          << (!analysis.beatsAttempted
                                  ? "skipped"
                                  : (outcome.modelAvailable ? "ready" : "unavailable"))
                          << "leading_silence_end_ms" << analysis.leadingSilenceEndMs
                          << "trailing_silence_start_ms" << analysis.trailingSilenceStartMs
                          << "integrated_lufs" << analysis.integratedLufs << "fade_out_start_ms"
                          << analysis.fadeOutStartMs;
    logRegions(analysis);
    Q_EMIT ready(analysis);
}

}
