#pragma once

#include <QNetworkCookieJar>
#include <QTimer>

namespace net {

class CookieJar : public QNetworkCookieJar
{
    Q_OBJECT

public:
    explicit CookieJar(QObject *parent = nullptr);
    ~CookieJar() override;

    bool setCookiesFromUrl(const QList<QNetworkCookie> &cookies, const QUrl &url) override;

    QString value(const QString &name) const;
    bool adopt(const QList<QNetworkCookie> &cookies);
    void discard();

Q_SIGNALS:
    void changed();

private:
    void load();
    void save();

    QTimer m_saveTimer;
    QString m_path;
};

}
