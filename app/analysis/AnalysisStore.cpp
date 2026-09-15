#include "AnalysisStore.h"

#include "core/Logging.h"
#include "core/Paths.h"

#include <QCborArray>
#include <QCborMap>
#include <QCborParserError>
#include <QCborValue>
#include <QDir>
#include <QFile>
#include <QSaveFile>

namespace {

constexpr int kAnalysisVersion = analysis::TrackAnalysis::kVersion;

bool safeId(const QString &videoId)
{
    for (const QChar character : videoId) {
        if (!character.isLetterOrNumber() && character != QLatin1Char('_')
            && character != QLatin1Char('-'))
            return false;
    }
    return !videoId.isEmpty();
}

QString directory()
{
    const QString path = core::paths::cacheDir() + QStringLiteral("/analysis");
    QDir().mkpath(path);
    return path;
}

QString pathFor(const QString &videoId, int itag)
{
    if (!safeId(videoId) || itag < 0)
        return {};
    return directory() + QLatin1Char('/') + videoId + QLatin1Char('-') + QString::number(itag)
        + QStringLiteral(".cbor");
}

QCborArray encodeCurve(const QList<double> &curve)
{
    QCborArray values;
    for (const double value : curve)
        values.append(value);
    return values;
}

std::optional<QList<double>> decodeCurve(const QCborValue &value)
{
    if (!value.isArray())
        return std::nullopt;

    QList<double> curve;
    const QCborArray values = value.toArray();
    curve.reserve(values.size());

    for (const QCborValue &entry : values) {
        if (!entry.isDouble() && !entry.isInteger())
            return std::nullopt;

        curve.append(entry.toDouble());
    }

    return curve;
}

QCborArray encodeTimes(const QList<qint64> &times)
{
    QCborArray values;
    for (const qint64 time : times)
        values.append(time);

    return values;
}

std::optional<QList<qint64>> decodeTimes(const QCborValue &value)
{
    if (!value.isArray())
        return std::nullopt;

    QList<qint64> times;
    const QCborArray values = value.toArray();
    times.reserve(values.size());

    for (const QCborValue &entry : values) {
        if (!entry.isInteger())
            return std::nullopt;

        times.append(entry.toInteger());
    }

    return times;
}

QCborArray encodeRegions(const QList<analysis::BeatRegion> &regions)
{
    QCborArray values;

    for (const analysis::BeatRegion &region : regions) {
        values.append(QCborMap {
            {QStringLiteral("startMs"), region.startMs},
            {QStringLiteral("endMs"), region.endMs},
            {QStringLiteral("beatsMs"), encodeTimes(region.beatsMs)},
            {QStringLiteral("downbeatsMs"), encodeTimes(region.downbeatsMs)},
            {QStringLiteral("tempo"), region.tempo},
            {QStringLiteral("phaseMs"), region.phaseMs},
            {QStringLiteral("beatConfidence"), region.beatConfidence},
            {QStringLiteral("downbeatConfidence"), region.downbeatConfidence},
        });
    }
    return values;
}

std::optional<QList<analysis::BeatRegion>> decodeRegions(const QCborValue &value)
{
    QList<analysis::BeatRegion> regions;
    if (value.isUndefined())
        return regions;

    if (!value.isArray())
        return std::nullopt;

    for (const QCborValue &entry : value.toArray()) {
        if (!entry.isMap())
            return std::nullopt;

        const QCborMap map = entry.toMap();
        const std::optional<QList<qint64>> beats =
            decodeTimes(map.value(QStringLiteral("beatsMs")));

        const std::optional<QList<qint64>> downbeats =
            decodeTimes(map.value(QStringLiteral("downbeatsMs")));

        if (!beats || !downbeats)
            return std::nullopt;

        analysis::BeatRegion region;
        region.startMs = map.value(QStringLiteral("startMs")).toInteger();
        region.endMs = map.value(QStringLiteral("endMs")).toInteger();
        region.beatsMs = *beats;
        region.downbeatsMs = *downbeats;
        region.tempo = map.value(QStringLiteral("tempo")).toDouble();
        region.phaseMs = map.value(QStringLiteral("phaseMs")).toDouble();
        region.beatConfidence = map.value(QStringLiteral("beatConfidence")).toDouble();
        region.downbeatConfidence = map.value(QStringLiteral("downbeatConfidence")).toDouble();

        if (!region.valid())
            return std::nullopt;

        regions.append(region);
    }
    return regions;
}

QCborArray encodeStructures(const QList<analysis::Structure::Result> &structures)
{
    QCborArray values;

    for (const analysis::Structure::Result &structure : structures) {
        values.append(QCborMap {
            {QStringLiteral("barMs"), structure.barMs},
            {QStringLiteral("beatsPerBar"), structure.beatsPerBar},
            {QStringLiteral("barsMs"), encodeTimes(structure.barsMs)},
            {QStringLiteral("lowDb"), encodeCurve(structure.lowDb)},
            {QStringLiteral("midDb"), encodeCurve(structure.midDb)},
            {QStringLiteral("highDb"), encodeCurve(structure.highDb)},
            {QStringLiteral("phrasesMs"), encodeTimes(structure.phrasesMs)},
            {QStringLiteral("introEndMs"), structure.introEndMs},
            {QStringLiteral("outroStartMs"), structure.outroStartMs},
            {QStringLiteral("musicalEndMs"), structure.musicalEndMs},
            {QStringLiteral("bassShare"), structure.bassShare},
        });
    }
    return values;
}

std::optional<QList<analysis::Structure::Result>> decodeStructures(const QCborValue &value)
{
    QList<analysis::Structure::Result> structures;
    if (value.isUndefined())
        return structures;

    if (!value.isArray())
        return std::nullopt;

    for (const QCborValue &entry : value.toArray()) {
        if (!entry.isMap())
            return std::nullopt;

        const QCborMap map = entry.toMap();
        const std::optional<QList<qint64>> bars = decodeTimes(map.value(QStringLiteral("barsMs")));
        const std::optional<QList<qint64>> phrases =
            decodeTimes(map.value(QStringLiteral("phrasesMs")));

        const std::optional<QList<double>> low = decodeCurve(map.value(QStringLiteral("lowDb")));
        const std::optional<QList<double>> mid = decodeCurve(map.value(QStringLiteral("midDb")));
        const std::optional<QList<double>> high = decodeCurve(map.value(QStringLiteral("highDb")));

        if (!bars || !phrases || !low || !mid || !high)
            return std::nullopt;

        analysis::Structure::Result structure;
        structure.barMs = map.value(QStringLiteral("barMs")).toInteger();
        structure.beatsPerBar = int(map.value(QStringLiteral("beatsPerBar")).toInteger());
        structure.barsMs = *bars;
        structure.lowDb = *low;
        structure.midDb = *mid;
        structure.highDb = *high;
        structure.phrasesMs = *phrases;
        structure.introEndMs = map.value(QStringLiteral("introEndMs")).toInteger(-1);
        structure.outroStartMs = map.value(QStringLiteral("outroStartMs")).toInteger(-1);
        structure.musicalEndMs = map.value(QStringLiteral("musicalEndMs")).toInteger(-1);
        structure.bassShare = map.value(QStringLiteral("bassShare")).toDouble();

        if (!structure.valid())
            return std::nullopt;

        structures.append(structure);
    }
    return structures;
}

QCborArray encodeKeys(const QList<analysis::KeyEstimator::Result> &keys)
{
    QCborArray values;
    for (const analysis::KeyEstimator::Result &key : keys) {
        values.append(QCborMap {
            {QStringLiteral("tonic"), key.tonic},
            {QStringLiteral("minor"), key.minor},
            {QStringLiteral("confidence"), key.confidence},
        });
    }
    return values;
}

std::optional<QList<analysis::KeyEstimator::Result>> decodeKeys(const QCborValue &value)
{
    QList<analysis::KeyEstimator::Result> keys;
    if (value.isUndefined())
        return keys;

    if (!value.isArray())
        return std::nullopt;

    for (const QCborValue &entry : value.toArray()) {
        if (!entry.isMap())
            return std::nullopt;

        const QCborMap map = entry.toMap();
        analysis::KeyEstimator::Result key;

        key.tonic = int(map.value(QStringLiteral("tonic")).toInteger(-1));
        key.minor = map.value(QStringLiteral("minor")).toBool();
        key.confidence = map.value(QStringLiteral("confidence")).toDouble();

        if (!key.valid())
            return std::nullopt;

        keys.append(key);
    }
    return keys;
}

}

namespace analysis {

std::optional<TrackAnalysis> AnalysisStore::load(const QString &videoId, int itag) const
{
    const QString path = pathFor(videoId, itag);
    if (path.isEmpty())
        return std::nullopt;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return std::nullopt;

    QCborParserError error;
    const QCborValue value = QCborValue::fromCbor(file.readAll(), &error);
    if (error.error != QCborError::NoError || !value.isMap()) {
        qCWarning(logTransition) << "cannot read analysis" << path;
        return std::nullopt;
    }

    const QCborMap map = value.toMap();
    TrackAnalysis analysis;
    analysis.version = int(map.value(QStringLiteral("version")).toInteger());
    analysis.videoId = map.value(QStringLiteral("videoId")).toString();
    analysis.itag = int(map.value(QStringLiteral("itag")).toInteger());
    analysis.durationMs = map.value(QStringLiteral("durationMs")).toInteger();
    analysis.leadingSilenceEndMs = map.value(QStringLiteral("leadingSilenceEndMs")).toInteger();
    analysis.trailingSilenceStartMs =
        map.value(QStringLiteral("trailingSilenceStartMs")).toInteger();
    analysis.fadeOutStartMs = map.value(QStringLiteral("fadeOutStartMs")).toInteger(-1);
    analysis.integratedLufs = map.value(QStringLiteral("integratedLufs")).toDouble(-70.0);
    const std::optional<QList<double>> curve =
        decodeCurve(map.value(QStringLiteral("shortTermLufs")));
    if (!curve)
        return std::nullopt;
    analysis.shortTermLufs = *curve;
    analysis.beatsAttempted = map.value(QStringLiteral("beatsAttempted")).toBool();
    analysis.beatsAvailable = map.value(QStringLiteral("beatsAvailable")).toBool();
    const std::optional<QList<BeatRegion>> regions =
        decodeRegions(map.value(QStringLiteral("beatRegions")));
    if (!regions)
        return std::nullopt;
    analysis.beatRegions = *regions;
    const std::optional<QList<Structure::Result>> structures =
        decodeStructures(map.value(QStringLiteral("structures")));
    const std::optional<QList<KeyEstimator::Result>> keys =
        decodeKeys(map.value(QStringLiteral("keys")));
    if (!structures || !keys)
        return std::nullopt;
    analysis.structures = *structures;
    analysis.keys = *keys;

    if (analysis.version != kAnalysisVersion || analysis.videoId != videoId || analysis.itag != itag
        || !analysis.valid())
        return std::nullopt;
    return analysis;
}

bool AnalysisStore::save(const TrackAnalysis &analysis) const
{
    const QString path = pathFor(analysis.videoId, analysis.itag);
    if (path.isEmpty() || !analysis.valid())
        return false;

    const QCborMap map {
        {QStringLiteral("version"), analysis.version},
        {QStringLiteral("videoId"), analysis.videoId},
        {QStringLiteral("itag"), analysis.itag},
        {QStringLiteral("durationMs"), analysis.durationMs},
        {QStringLiteral("leadingSilenceEndMs"), analysis.leadingSilenceEndMs},
        {QStringLiteral("trailingSilenceStartMs"), analysis.trailingSilenceStartMs},
        {QStringLiteral("fadeOutStartMs"), analysis.fadeOutStartMs},
        {QStringLiteral("integratedLufs"), analysis.integratedLufs},
        {QStringLiteral("shortTermLufs"), encodeCurve(analysis.shortTermLufs)},
        {QStringLiteral("beatsAttempted"), analysis.beatsAttempted},
        {QStringLiteral("beatsAvailable"), analysis.beatsAvailable},
        {QStringLiteral("beatRegions"), encodeRegions(analysis.beatRegions)},
        {QStringLiteral("structures"), encodeStructures(analysis.structures)},
        {QStringLiteral("keys"), encodeKeys(analysis.keys)},
    };
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    const QByteArray bytes = QCborValue(map).toCbor();
    return file.write(bytes) == bytes.size() && file.commit();
}

}
