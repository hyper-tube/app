#pragma once

#include "ColorSource.h"

#include <QList>
#include <QObject>
#include <QQmlEngine>

namespace theme {

class SystemTheme : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(QString sourceName READ sourceName NOTIFY changed)
    Q_PROPERTY(QList<theme::ColorSourceInfo> sources READ sources NOTIFY changed)

public:
    explicit SystemTheme(QObject *parent);

    static SystemTheme &instance();
    static SystemTheme *create(QQmlEngine *, QJSEngine *);

    bool available() const { return active() != nullptr; }
    bool complete() const;
    bool dark() const;
    QString sourceName() const;
    QList<ColorSourceInfo> sources() const;
    const ColorSource::Roles &roles() const;
    QColor accent() const;

    Q_INVOKABLE void select(const QString &id);

Q_SIGNALS:
    void changed();

private:
    void discover();
    const ColorSource *active() const;
    Qt::ColorScheme colorScheme() const;

    QList<ColorSource *> m_sources;
    Qt::ColorScheme m_platformScheme = Qt::ColorScheme::Unknown;
    QString m_preferredId;
};

}
