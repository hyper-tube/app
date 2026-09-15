#pragma once

#include <QDateTime>
#include <QMetaType>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace control {

class Notification
{
    Q_GADGET
    QML_VALUE_TYPE(notification)

    Q_PROPERTY(QString id MEMBER id)
    Q_PROPERTY(QString icon MEMBER icon)
    Q_PROPERTY(QString title MEMBER title)
    Q_PROPERTY(QString body MEMBER body)
    Q_PROPERTY(QString action MEMBER action)
    Q_PROPERTY(QString actionIcon MEMBER actionIcon)
    Q_PROPERTY(double progress MEMBER progress)
    Q_PROPERTY(bool unread MEMBER unread)
    Q_PROPERTY(bool activity MEMBER activity)
    Q_PROPERTY(bool dismissible READ dismissible)
    Q_PROPERTY(QString age READ age)

public:
    QString id;
    QString icon;
    QString title;
    QString body;
    QString action;
    QString actionIcon;
    QDateTime postedAt;
    double progress = 0;
    bool activity = false;
    bool unread = false;
    bool quiet = false;

    bool dismissible() const { return !activity; }

    QString age() const
    {
        if (!postedAt.isValid())
            return {};
        const qint64 seconds = postedAt.secsTo(QDateTime::currentDateTime());
        if (seconds < 90)
            return QStringLiteral("just now");
        if (seconds < 5400)
            return QString::number(seconds / 60) + QStringLiteral("m ago");
        if (seconds < 129600)
            return QString::number(seconds / 3600) + QStringLiteral("h ago");
        return QString::number(seconds / 86400) + QStringLiteral("d ago");
    }

    bool operator==(const Notification &other) const { return id == other.id; }
};

}

Q_DECLARE_METATYPE(control::Notification)
