#pragma once

#include "Job.h"
#include "Options.h"
#include "Registration.h"

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QStringList>
#include <QtQmlIntegration>

namespace setup {

class Session : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY changed)
    Q_PROPERTY(Mode maintenanceMode READ maintenanceMode NOTIFY changed)
    Q_PROPERTY(Scope scope READ scope WRITE setScope NOTIFY changed)
    Q_PROPERTY(QString directory READ directory WRITE setDirectory NOTIFY directoryChanged)
    Q_PROPERTY(bool desktopShortcut READ desktopShortcut WRITE setDesktopShortcut NOTIFY changed)
    Q_PROPERTY(
        bool startMenuShortcut READ startMenuShortcut WRITE setStartMenuShortcut NOTIFY changed)
    Q_PROPERTY(bool launchAtSignIn READ launchAtSignIn WRITE setLaunchAtSignIn NOTIFY changed)
    Q_PROPERTY(bool urlScheme READ urlScheme WRITE setUrlScheme NOTIFY changed)
    Q_PROPERTY(bool launchWhenDone READ launchWhenDone WRITE setLaunchWhenDone NOTIFY changed)
    Q_PROPERTY(bool removeUserData READ removeUserData WRITE setRemoveUserData NOTIFY changed)

    Q_PROPERTY(QString productName READ productName CONSTANT)
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(QString publisher READ publisher CONSTANT)
    Q_PROPERTY(QString website READ website CONSTANT)
    Q_PROPERTY(QString licenseText READ licenseText CONSTANT)
    Q_PROPERTY(QString urlSchemeName READ urlSchemeName CONSTANT)

    Q_PROPERTY(QString installedVersion READ installedVersion NOTIFY changed)
    Q_PROPERTY(QString installedDirectory READ installedDirectory NOTIFY changed)
    Q_PROPERTY(bool elevationNeeded READ elevationNeeded NOTIFY changed)
    Q_PROPERTY(bool elevated READ elevated CONSTANT)

    Q_PROPERTY(qint64 requiredBytes READ requiredBytes NOTIFY changed)
    Q_PROPERTY(qint64 availableBytes READ availableBytes NOTIFY directoryChanged)
    Q_PROPERTY(QString directoryComplaint READ directoryComplaint NOTIFY directoryChanged)
    Q_PROPERTY(QStringList blockingProcesses READ blockingProcesses NOTIFY blockersChanged)

    Q_PROPERTY(QString complaint READ complaint NOTIFY changed)
    Q_PROPERTY(QString logFile READ logFile CONSTANT)
    Q_PROPERTY(QStringList userDataDirectories READ userDataDirectories CONSTANT)
    Q_PROPERTY(bool downgrade READ downgrade NOTIFY changed)
    Q_PROPERTY(bool automatic READ automatic CONSTANT)
    Q_PROPERTY(Job *job READ job CONSTANT)

public:
    explicit Session(QObject *parent);

    static Session &instance();
    static Session *create(QQmlEngine *, QJSEngine *);

    void adopt(const Options &options);
    bool claim();
    void release();
    QString complaint() const { return m_complaint; }
    QString logFile() const;
    QStringList userDataDirectories() const;
    bool downgrade() const;
    bool automatic() const { return m_options.resume || m_options.silent; }
    Outcome result() const { return m_result; }
    void setMode(Mode mode);
    const Options &options() const { return m_options; }

    Mode mode() const { return m_options.mode; }
    Mode maintenanceMode() const;
    Scope scope() const { return m_options.scope; }
    QString directory() const;
    bool desktopShortcut() const;
    bool startMenuShortcut() const;
    bool launchAtSignIn() const;
    bool urlScheme() const;
    bool launchWhenDone() const { return m_options.launchWhenDone; }
    bool removeUserData() const { return m_options.removeUserData; }

    QString productName() const;
    QString version() const;
    QString publisher() const;
    QString website() const;
    QString licenseText() const;
    QString urlSchemeName() const;

    QString installedVersion() const { return m_installed.version; }
    QString installedDirectory() const { return m_installed.directory; }
    bool elevationNeeded() const;
    bool elevated() const { return m_options.elevated; }

    qint64 requiredBytes() const { return qint64(m_required); }
    qint64 availableBytes() const;
    QString directoryComplaint() const;
    QStringList blockingProcesses() const { return m_blockers; }

    Job *job() { return &m_job; }

    void setScope(Scope scope);
    void setDirectory(const QString &directory);
    void setDesktopShortcut(bool on);
    void setStartMenuShortcut(bool on);
    void setLaunchAtSignIn(bool on);
    void setUrlScheme(bool on);
    void setLaunchWhenDone(bool on);
    void setRemoveUserData(bool on);

    Q_INVOKABLE void chooseDirectory(const QUrl &url);
    Q_INVOKABLE void copyDetails(const QString &message);
    Q_INVOKABLE void finish(int outcome);
    Q_INVOKABLE void refreshBlockers();
    Q_INVOKABLE bool closeBlockers();
    Q_INVOKABLE bool start();
    Q_INVOKABLE void launchApplication();
    Q_INVOKABLE QString readable(qint64 bytes) const;

Q_SIGNALS:
    void changed();
    void directoryChanged();
    void blockersChanged();
    void elevationDeclined();

private:
    void setComponent(Component component, bool on);
    bool elevate();

    Options m_options;
    Installed m_installed;
    Job m_job;
    quint64 m_required = 0;
    QStringList m_blockers;
    QString m_complaint;
    Outcome m_result = Outcome::Cancelled;
    void *m_mutex = nullptr;
};

}
