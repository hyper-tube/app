#pragma once

#include "ImageCache.h"
#include "media/Track.h"
#include "net/DownloadTransfer.h"
#include "player/StreamResolver.h"

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QQmlEngine>
#include <QSet>
#include <QTimer>

#include <optional>

namespace library {

class Downloads : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QStringList ids READ ids NOTIFY changed)
    Q_PROPERTY(bool smartEnabled READ smartEnabled WRITE setSmartEnabled NOTIFY settingsChanged)
    Q_PROPERTY(int trackLimit READ trackLimit WRITE setTrackLimit NOTIFY settingsChanged)
    Q_PROPERTY(double sizeLimitGb READ sizeLimitGb WRITE setSizeLimitGb NOTIFY settingsChanged)
    Q_PROPERTY(double partialBytes READ partialBytes NOTIFY progressChanged)
    Q_PROPERTY(double smartBytes READ smartBytes NOTIFY changed)
    Q_PROPERTY(double forcedBytes READ forcedBytes NOTIFY changed)
    Q_PROPERTY(int pendingCount READ pendingCount NOTIFY changed)
    Q_PROPERTY(int smartPending READ smartPending NOTIFY changed)
    Q_PROPERTY(QString activeTitle READ activeTitle NOTIFY changed)
    Q_PROPERTY(bool activeForced READ activeForced NOTIFY changed)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(double fileProgress READ fileProgress NOTIFY progressChanged)
    Q_PROPERTY(int batchTotal READ batchTotal NOTIFY progressChanged)
    Q_PROPERTY(int batchDone READ batchDone NOTIFY progressChanged)

public:
    enum State {
        Absent,
        Waiting,
        Fetching,
        Ready,
        Failed,
    };
    Q_ENUM(State)

    explicit Downloads(QObject *parent);

    static Downloads &instance();
    static Downloads *create(QQmlEngine *, QJSEngine *);

    QStringList ids() const;
    QList<media::Track> tracks() const;
    std::optional<player::Stream> localStream(const media::Track &track) const;
    bool smartEnabled() const { return m_smartEnabled; }
    int trackLimit() const { return m_trackLimit; }
    double sizeLimitGb() const { return m_sizeLimitGb; }
    double partialBytes() const;
    double smartBytes() const;
    double forcedBytes() const;
    int pendingCount() const { return m_wanted.size(); }
    int smartPending() const;
    QString activeTitle() const { return m_wanted.value(m_activeVideoId).title; }
    bool activeForced() const { return m_forced.contains(m_activeVideoId); }
    double progress() const;
    double fileProgress() const { return m_progress; }
    int batchTotal() const { return m_batchTotal; }
    int batchDone() const { return m_batchDone; }
    void setSmartEnabled(bool enabled);
    void setTrackLimit(int count);
    void setSizeLimitGb(double gigabytes);

    Q_INVOKABLE bool isForced(const QString &videoId) const;
    Q_INVOKABLE int stateOf(const QString &videoId) const;
    Q_INVOKABLE void keep(const QList<media::Track> &tracks);
    Q_INVOKABLE void discard(const QList<media::Track> &tracks);
    Q_INVOKABLE void cleanUp();
    Q_INVOKABLE void removeAll();
    Q_INVOKABLE void browseFiles() const;

Q_SIGNALS:
    void changed();
    void catalogueChanged();
    void settingsChanged();
    void progressChanged();
    void feedback(const QString &message);
    void smartFailed(const QString &title);

private:
    struct Entry
    {
        media::Track track;
        QString file;
        qint64 bytes = 0;
        qint64 fetchedAt = 0;
        int itag = 0;
        double gainDb = 0.0;
        bool hasGain = false;
        bool forced = true;
    };

    QString directory() const;
    QString indexFile() const;
    QString partialFile(const QString &videoId) const;
    void load();
    bool save() const;
    void saveSettings();
    void enqueue(const QList<media::Track> &tracks, bool forced);
    void openBatch(int added);
    void closeBatchItem();
    void dropFromBatch(const QString &videoId);
    void pump();
    void fetch(const player::Stream &stream);
    void store(const player::Stream &stream);
    void storeArtwork(const QString &videoId);
    void reserveSpace(qint64 bytes);
    void fail(const QString &videoId, const QString &message);
    void refuseLive(const QString &videoId);
    void defer(const QString &videoId);
    void restoreArtwork();
    void cancel(const QString &videoId);
    void removeReleased();
    qint64 ceiling() const;

    ImageCache m_artworkCache;
    player::StreamResolver m_resolver;
    QPointer<net::DownloadTransfer> m_transfer;
    QHash<QString, Entry> m_entries;
    QHash<QString, media::Track> m_wanted;
    QHash<QString, State> m_states;
    QHash<QString, int> m_partialItags;
    QHash<QString, qint64> m_retryAfter;
    QSet<QString> m_forced;
    QSet<QString> m_removals;
    QSet<QString> m_excluded;
    QList<QString> m_queue;
    QString m_activeVideoId;
    QTimer m_pumpTimer;
    QTimer m_plannerTimer;
    QTimer m_replanTimer;
    double m_progress = 0;
    int m_batchTotal = 0;
    int m_batchDone = 0;
    double m_sizeLimitGb = 2;
    int m_trackLimit = 200;
    bool m_smartEnabled = true;
};

}
