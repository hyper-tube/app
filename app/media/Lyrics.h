#pragma once

#include "innertube/Endpoints.h"

#include <QList>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QStringList>

namespace media {

class Lyrics : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QStringList lines READ lines NOTIFY changed)
    Q_PROPERTY(bool synced READ synced NOTIFY changed)
    Q_PROPERTY(bool loading READ loading NOTIFY changed)
    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(bool interrupted READ interrupted NOTIFY changed)
    Q_PROPERTY(QString source READ source NOTIFY changed)
    Q_PROPERTY(int activeLine READ activeLine NOTIFY activeLineChanged)

public:
    explicit Lyrics(QObject *parent);

    static Lyrics &instance();
    static Lyrics *create(QQmlEngine *, QJSEngine *);

    const QStringList &lines() const { return m_lines; }
    bool synced() const { return !m_starts.isEmpty(); }
    bool loading() const { return m_loading; }
    bool available() const { return !m_lines.isEmpty(); }
    bool interrupted() const { return m_interrupted; }
    QString source() const { return m_source; }
    int activeLine() const { return m_activeLine; }

    Q_INVOKABLE void retry();

Q_SIGNALS:
    void changed();
    void activeLineChanged();

private:
    void refresh();
    void load();
    void interrupt();
    void discard();
    void findBrowseId(const QString &videoId, quint64 generation);
    void fetchTimed(const QString &browseId, quint64 generation);
    void fetchPlain(const QString &browseId, quint64 generation);
    bool acceptTimed(const innertube::Reply &reply);
    bool acceptPlain(const innertube::Reply &reply);
    void publish();
    void syncTo(qint64 milliseconds);
    void setLoading(bool loading);
    void setActiveLine(int line);

    innertube::Endpoints m_endpoints;
    QStringList m_lines;
    QList<qint64> m_starts;
    QString m_videoId;
    QString m_source;
    quint64 m_generation = 0;
    int m_activeLine = -1;
    bool m_loading = false;
    bool m_interrupted = false;
};

}
