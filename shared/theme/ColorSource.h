#pragma once

#include "ColorSourceInfo.h"

#include <QColor>
#include <QHash>
#include <QObject>
#include <QString>

namespace theme {

class ColorSource : public QObject
{
    Q_OBJECT

public:
    using Roles = QHash<QString, QColor>;

    ColorSource(QString id, QString name, QString location, QObject *parent);

    const QString &id() const { return m_id; }
    const QString &name() const { return m_name; }
    const QString &location() const { return m_location; }
    const Roles &roles() const { return m_roles; }
    const QColor &accent() const { return m_accent; }
    Qt::ColorScheme colorScheme() const { return m_colorScheme; }

    bool available() const { return m_accent.isValid(); }
    bool complete() const { return !m_roles.isEmpty(); }
    ColorSourceInfo info(bool active) const;

public Q_SLOTS:
    virtual void refresh() = 0;

Q_SIGNALS:
    void changed();

protected:
    static Qt::ColorScheme schemeFor(const QColor &background);

    void adopt(const Roles &roles, const QColor &accent, Qt::ColorScheme colorScheme);
    void clear();

private:
    QString m_id;
    QString m_name;
    QString m_location;
    Roles m_roles;
    QColor m_accent;
    Qt::ColorScheme m_colorScheme = Qt::ColorScheme::Unknown;
};

}
