#pragma once

#include "innertube/Endpoints.h"

#include <QObject>
#include <QQmlEngine>
#include <QString>

namespace media {

class EpisodeDetails : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString videoId READ videoId NOTIFY changed)
    Q_PROPERTY(QString title READ title NOTIFY changed)
    Q_PROPERTY(QString podcast READ podcast NOTIFY changed)
    Q_PROPERTY(QString podcastId READ podcastId NOTIFY changed)
    Q_PROPERTY(QString artId READ artId NOTIFY changed)
    Q_PROPERTY(QString meta READ meta NOTIFY changed)
    Q_PROPERTY(QString description READ description NOTIFY changed)
    Q_PROPERTY(bool loading READ loading NOTIFY changed)
    Q_PROPERTY(bool failed READ failed NOTIFY changed)
    Q_PROPERTY(bool unreachable READ unreachable NOTIFY changed)

public:
    explicit EpisodeDetails(QObject *parent);

    static EpisodeDetails &instance();
    static EpisodeDetails *create(QQmlEngine *, QJSEngine *);

    const QString &videoId() const { return m_videoId; }
    const QString &title() const { return m_title; }
    const QString &podcast() const { return m_podcast; }
    const QString &podcastId() const { return m_podcastId; }
    const QString &artId() const { return m_artId; }
    const QString &meta() const { return m_meta; }
    const QString &description() const { return m_description; }
    bool loading() const { return m_loading; }
    bool failed() const { return m_failed; }
    bool unreachable() const { return m_unreachable; }

    Q_INVOKABLE void retry();
    Q_INVOKABLE void activate(const QString &link) const;

Q_SIGNALS:
    void changed();

private:
    void follow();
    void fetch();
    void accept(const innertube::Reply &reply, quint64 generation);
    void clear();

    innertube::Endpoints m_endpoints;
    QString m_videoId;
    QString m_title;
    QString m_podcast;
    QString m_podcastId;
    QString m_artId;
    QString m_meta;
    QString m_description;
    quint64 m_generation = 0;
    bool m_loading = false;
    bool m_failed = false;
    bool m_unreachable = false;
};

}
