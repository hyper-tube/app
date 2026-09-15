#pragma once

#include "PackageDownload.h"
#include "Release.h"
#include "ReleaseSection.h"
#include "SemanticVersion.h"

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QQmlEngine>
#include <QString>
#include <QTimer>
#include <QUrl>

#include <optional>

namespace net {
struct Response;
}

namespace update {

class Updater : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool available READ available NOTIFY releaseChanged)
    Q_PROPERTY(QString version READ version NOTIFY releaseChanged)
    Q_PROPERTY(QString published READ published NOTIFY releaseChanged)
    Q_PROPERTY(bool prerelease READ prerelease NOTIFY releaseChanged)
    Q_PROPERTY(QString notes READ notes NOTIFY releaseChanged)
    Q_PROPERTY(QList<update::ReleaseSection> sections READ sections NOTIFY releaseChanged)
    Q_PROPERTY(QUrl changelogUrl READ changelogUrl NOTIFY releaseChanged)
    Q_PROPERTY(QUrl downloadPage READ downloadPage NOTIFY releaseChanged)
    Q_PROPERTY(bool installable READ installable NOTIFY releaseChanged)
    Q_PROPERTY(CheckState checkState READ checkState NOTIFY checkStateChanged)
    Q_PROPERTY(Stage stage READ stage NOTIFY stageChanged)
    Q_PROPERTY(QString error READ error NOTIFY stageChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString transferred READ transferred NOTIFY progressChanged)
    Q_PROPERTY(bool automatic READ automatic WRITE setAutomatic NOTIFY automaticChanged)

public:
    enum CheckState {
        NotChecked,
        Checking,
        Checked,
        Unreachable,
    };
    Q_ENUM(CheckState)

    enum Stage {
        Idle,
        Downloading,
        Installing,
        Failed,
    };
    Q_ENUM(Stage)

    explicit Updater(QObject *parent);

    static Updater &instance();
    static Updater *create(QQmlEngine *, QJSEngine *);

    void start();

    bool available() const { return m_release.has_value(); }
    QString version() const;
    QString published() const;
    bool prerelease() const;
    QString notes() const;
    const QList<ReleaseSection> &sections() const { return m_sections; }
    QUrl changelogUrl() const;
    QUrl downloadPage() const;
    bool installable() const;
    CheckState checkState() const { return m_checkState; }
    Stage stage() const { return m_stage; }
    const QString &error() const { return m_error; }
    double progress() const;
    QString transferred() const;
    bool automatic() const { return m_automatic; }
    void setAutomatic(bool automatic);

    Q_INVOKABLE void check();
    Q_INVOKABLE void install();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void showDetails();

Q_SIGNALS:
    void releaseChanged();
    void checkStateChanged();
    void stageChanged();
    void progressChanged();
    void automaticChanged();
    void detailsRequested();

private:
    bool busy() const;
    QString channel() const;
    QString noticeId() const;
    void schedule();
    void checkIfDue();
    void receive(const net::Response &response);
    void adopt(std::optional<Release> release);
    void retractNotice();
    void announce();
    void rememberSeen() const;
    void publishProgress();
    void conclude(update::PackageDownload::Failure failure);
    void handOff(const QString &package);
    void reportUnfinished(const QString &version);
    void setCheckState(CheckState state);
    void setStage(Stage stage, const QString &error = {});

    SemanticVersion m_running;
    std::optional<Release> m_release;
    QList<ReleaseSection> m_sections;
    QPointer<PackageDownload> m_download;
    QTimer m_schedule;
    QDateTime m_checkedAt;
    QByteArray m_entityTag;
    QString m_error;
    qint64 m_received = 0;
    CheckState m_checkState = NotChecked;
    Stage m_stage = Idle;
    bool m_automatic = true;
};

}
