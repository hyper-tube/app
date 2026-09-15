#pragma once

#include <QFile>
#include <QList>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QStringList>
#include <QUrl>

namespace library {

class Uploads : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString activeName READ activeName NOTIFY changed)
    Q_PROPERTY(int batchTotal READ batchTotal NOTIFY changed)
    Q_PROPERTY(int batchDone READ batchDone NOTIFY changed)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QStringList extensions READ extensions CONSTANT)

public:
    explicit Uploads(QObject *parent);

    static Uploads &instance();
    static Uploads *create(QQmlEngine *, QJSEngine *);

    QString activeName() const { return m_activeName; }
    int batchTotal() const { return m_batchTotal; }
    int batchDone() const { return m_batchDone; }
    double progress() const;
    QStringList extensions() const;

    Q_INVOKABLE void add(const QList<QUrl> &files);

Q_SIGNALS:
    void changed();
    void progressChanged();
    void feedback(const QString &message, bool error);

private:
    void pump();
    void open(const QString &path);
    void send(const QString &path, const QUrl &target);
    void fail(const QString &name, const QString &reason);
    void complete();

    QFile m_file;
    QList<QString> m_queue;
    QString m_activeName;
    double m_progress = 0;
    int m_batchTotal = 0;
    int m_batchDone = 0;
    int m_failures = 0;
};

}
