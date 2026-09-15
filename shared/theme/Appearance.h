#pragma once

#include <QColor>
#include <QObject>
#include <QQmlEngine>

namespace theme {

class Appearance : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY changed)
    Q_PROPERTY(QColor seed READ seed WRITE setSeed NOTIFY changed)
    Q_PROPERTY(bool followsSystemColor READ followsSystemColor NOTIFY changed)
    Q_PROPERTY(bool systemColorAvailable READ systemColorAvailable NOTIFY changed)

public:
    enum Mode {
        System,
        Light,
        Dark,
    };
    Q_ENUM(Mode)

    explicit Appearance(QObject *parent);

    static Appearance &instance();
    static Appearance *create(QQmlEngine *, QJSEngine *);

    Mode mode() const { return m_mode; }
    QColor seed() const;
    bool followsSystemColor() const;
    bool systemColorAvailable() const;
    bool dark() const;
    bool generates() const;

    void setMode(Mode mode);
    void setSeed(const QColor &seed);

    Q_INVOKABLE void followSystemColor();

Q_SIGNALS:
    void changed();

private:
    void save() const;

    Mode m_mode = System;
    QColor m_customSeed;
};

}
