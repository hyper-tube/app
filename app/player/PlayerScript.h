#pragma once

#include <QJSValue>
#include <QList>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QUrl>

#include <functional>
#include <memory>

class QJSEngine;

namespace player {

class PlayerScript : public QObject
{
    Q_OBJECT

public:
    using Handler = std::function<void(int)>;

    explicit PlayerScript(QObject *parent = nullptr);
    ~PlayerScript() override;

    static PlayerScript &instance();

    void ready(Handler handler);
    QUrl descramble(const QUrl &url);

private:
    QString cacheFile(const QString &version) const;
    void start();
    void fetchVersion();
    void fetchSource();
    void adoptSource(const QByteArray &source);
    bool build(const QString &source);
    void serve(int signatureTimestamp);

    std::unique_ptr<QJSEngine> m_engine;
    QJSValue m_transform;
    QString m_version;
    QList<Handler> m_pending;
    QTimer m_idle;
    int m_signatureTimestamp = 0;
    bool m_fetching = false;
};

}
