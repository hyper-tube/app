#pragma once

#include "innertube/Endpoints.h"
#include "model/Item.h"
#include "model/SortOption.h"

#include <QObject>
#include <QHash>
#include <QSet>
#include <QQmlEngine>
#include <QString>
#include <QStringList>

#include <functional>

namespace library {

class LibraryActions : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QStringList pendingRatings READ pendingRatings NOTIFY pendingRatingsChanged)

public:
    explicit LibraryActions(QObject *parent);

    static LibraryActions &instance();
    static LibraryActions *create(QQmlEngine *, QJSEngine *);

    const QString &error() const { return m_error; }
    QStringList pendingRatings() const { return m_ratingWrites.values(); }

    Q_INVOKABLE void setTrackLiked(const QString &videoId, bool liked);
    Q_INVOKABLE void rateTrack(const QString &videoId, int value);
    void rememberRating(const QString &videoId, int rating);
    void refreshTrack(const QString &videoId);
    int rating(const QString &videoId, int fallback) const;
    Q_INVOKABLE void setSaved(const QString &playlistId, bool saved);
    Q_INVOKABLE void setSubscribed(const QString &channelId, bool subscribed);
    Q_INVOKABLE void createPlaylist(const QString &title);
    void createPlaylistWith(const QString &title, const QStringList &videoIds);
    Q_INVOKABLE void deletePlaylist(const QString &playlistId);
    Q_INVOKABLE void togglePinned(const model::Item &item);
    Q_INVOKABLE void toggleInLibrary(const model::Item &item);
    Q_INVOKABLE void setQueuedForLater(const QString &videoId, bool queued);
    Q_INVOKABLE void setPlayed(const model::Item &item, bool played);
    Q_INVOKABLE void addToPlaylist(const QString &playlistId, const QString &playlistTitle,
                                   const QStringList &videoIds);
    Q_INVOKABLE void removeFromPlaylist(const QString &playlistId, const QStringList &videoIds,
                                        const QStringList &setVideoIds);
    Q_INVOKABLE void editPlaylist(const QString &playlistId, const QString &name,
                                  const QString &description, const QString &privacy);
    void movePlaylistItem(const QString &playlistId, const QString &setVideoId,
                          const QString &predecessor, const std::function<void(bool)> &handler);
    void setPlaylistOrder(const QString &playlistId, const model::SortOption &option,
                          const std::function<void(bool)> &handler);
    void applyOverrides(model::Item &item) const;

Q_SIGNALS:
    void errorChanged();
    void pendingRatingsChanged();
    void trackRatingChanged(const QString &videoId, int rating);
    void trackDisliked(const QString &videoId);
    void trackEntryRead(const model::Item &entry);
    void playlistDeleted(const QString &playlistId);
    void feedback(const QString &message);
    void savedChanged(const QString &playlistId, bool saved);
    void subscriptionChanged(const QString &channelId, bool subscribed);
    void pinChanged(const QString &key, bool pinned);
    void libraryChanged(const QString &key, bool inLibrary);
    void laterChanged(const QString &videoId, bool later);
    void playedChanged(const QString &videoId, bool played);
    void playlistItemsRemoved(const QString &playlistId, const QStringList &setVideoIds);
    void playlistEdited(const QString &playlistId, const QString &name, const QString &description,
                        const QString &privacy);
    void playlistReordered(const QString &playlistId);
    void collectionChanged();

private:
    using Revert = std::function<void()>;

    bool available();
    void send(const QString &endpoint, const QJsonObject &body, const QString &failure,
              const Revert &revert, const Revert &success = {});
    void edit(const QString &playlistId, const QJsonArray &actions, const QString &failure,
              const Revert &revert, const Revert &success = {});
    void setError(const QString &error);
    void forget();

    innertube::Endpoints m_endpoints;
    QString m_error;
    QHash<QString, int> m_ratings;
    QHash<QString, quint64> m_ratingVersions;
    QHash<QString, bool> m_pins;
    QHash<QString, bool> m_collected;
    QHash<QString, bool> m_later;
    QHash<QString, bool> m_played;
    QString m_accountId;
    QSet<QString> m_ratingWrites;
    quint64 m_accountGeneration = 0;
};

}
