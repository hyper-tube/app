#pragma once

#include <QObject>
#include <QString>

namespace auth {

class FirefoxAgent : public QObject
{
    Q_OBJECT

public:
    explicit FirefoxAgent(QObject *parent = nullptr);

    QString userAgent() const;
    void refresh();

Q_SIGNALS:
    void changed();

private:
    void adopt(const QString &version);

    QString m_version;
};

}
