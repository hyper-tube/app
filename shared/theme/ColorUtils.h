#pragma once

#include <QColor>
#include <QObject>
#include <QQmlEngine>

namespace theme {

class ColorUtils : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit ColorUtils(QObject *parent = nullptr);

    Q_INVOKABLE static QColor mix(const QColor &a, const QColor &b, qreal ratio);
    Q_INVOKABLE static QColor withAlpha(const QColor &source, qreal alpha);
    Q_INVOKABLE static QColor transparentize(const QColor &source, qreal amount);
    Q_INVOKABLE static QColor layer(const QColor &base, const QColor &overlay, qreal alpha);
    Q_INVOKABLE static qreal luminance(const QColor &source);
    Q_INVOKABLE static bool isDark(const QColor &source);
    Q_INVOKABLE static QColor contrasting(const QColor &background, const QColor &light,
                                          const QColor &dark);
};

}
