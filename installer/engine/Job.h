#pragma once

#include "Options.h"

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QtQmlIntegration>

#include <atomic>

class QThread;

namespace setup {

class Job : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("The job is owned by the session.")

    Q_PROPERTY(qreal progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString step READ step NOTIFY stepChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)

public:
    explicit Job(QObject *parent = nullptr);
    ~Job() override;

    qreal progress() const { return m_progress; }
    const QString &step() const { return m_step; }
    bool running() const { return m_running; }

    void install(const Options &options);
    void uninstall(const Options &options);

    Q_INVOKABLE void cancel();

    static quint64 requiredBytes(const Options &options);
    static bool enoughSpace(const QString &directory, quint64 needed);

Q_SIGNALS:
    void progressChanged();
    void stepChanged();
    void runningChanged();
    void finished(Outcome outcome, const QString &message);

private:
    void run(const Options &options, bool removing);
    bool performInstall(const Options &options, QString &message);
    bool performUninstall(const Options &options, QString &message);
    void report(const QString &step);
    void advance(qreal value);

    QThread *m_thread = nullptr;
    std::atomic_bool m_cancelled {false};
    qreal m_progress = 0;
    QString m_step;
    bool m_running = false;
};

}
