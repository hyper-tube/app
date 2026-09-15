#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QList>
#include <QTranslator>
#include <QVariantList>

namespace core {

class Localization : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY changed)
    Q_PROPERTY(QString resolved READ resolved NOTIFY resolvedChanged)
    Q_PROPERTY(QVariantList options READ options NOTIFY changed)

public:
    explicit Localization(QObject *parent);

    static Localization &instance();
    static Localization *create(QQmlEngine *, QJSEngine *);

    const QString &language() const { return m_language; }
    const QString &resolved() const { return m_resolved; }
    QVariantList options() const;

    void setLanguage(const QString &language);

Q_SIGNALS:
    void changed();
    void resolvedChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void adopt(const QString &resolved);

    QList<QTranslator *> m_catalogs;
    QString m_language;
    QString m_resolved;
};

}
