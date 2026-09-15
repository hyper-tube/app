#pragma once

#include <QColor>
#include <QObject>
#include <QQmlEngine>

namespace theme {

class Palette : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool darkMode READ darkMode NOTIFY changed)
    Q_PROPERTY(QColor background MEMBER m_background NOTIFY changed)
    Q_PROPERTY(QColor error MEMBER m_error NOTIFY changed)
    Q_PROPERTY(QColor errorContainer MEMBER m_errorContainer NOTIFY changed)
    Q_PROPERTY(QColor inverseOnSurface MEMBER m_inverseOnSurface NOTIFY changed)
    Q_PROPERTY(QColor inversePrimary MEMBER m_inversePrimary NOTIFY changed)
    Q_PROPERTY(QColor inverseSurface MEMBER m_inverseSurface NOTIFY changed)
    Q_PROPERTY(QColor onBackground MEMBER m_onBackground NOTIFY changed)
    Q_PROPERTY(QColor onError MEMBER m_onError NOTIFY changed)
    Q_PROPERTY(QColor onErrorContainer MEMBER m_onErrorContainer NOTIFY changed)
    Q_PROPERTY(QColor onPrimary MEMBER m_onPrimary NOTIFY changed)
    Q_PROPERTY(QColor onPrimaryContainer MEMBER m_onPrimaryContainer NOTIFY changed)
    Q_PROPERTY(QColor onPrimaryFixed MEMBER m_onPrimaryFixed NOTIFY changed)
    Q_PROPERTY(QColor onPrimaryFixedVariant MEMBER m_onPrimaryFixedVariant NOTIFY changed)
    Q_PROPERTY(QColor onSecondary MEMBER m_onSecondary NOTIFY changed)
    Q_PROPERTY(QColor onSecondaryContainer MEMBER m_onSecondaryContainer NOTIFY changed)
    Q_PROPERTY(QColor onSecondaryFixed MEMBER m_onSecondaryFixed NOTIFY changed)
    Q_PROPERTY(QColor onSecondaryFixedVariant MEMBER m_onSecondaryFixedVariant NOTIFY changed)
    Q_PROPERTY(QColor onSurface MEMBER m_onSurface NOTIFY changed)
    Q_PROPERTY(QColor onSurfaceVariant MEMBER m_onSurfaceVariant NOTIFY changed)
    Q_PROPERTY(QColor onTertiary MEMBER m_onTertiary NOTIFY changed)
    Q_PROPERTY(QColor onTertiaryContainer MEMBER m_onTertiaryContainer NOTIFY changed)
    Q_PROPERTY(QColor onTertiaryFixed MEMBER m_onTertiaryFixed NOTIFY changed)
    Q_PROPERTY(QColor onTertiaryFixedVariant MEMBER m_onTertiaryFixedVariant NOTIFY changed)
    Q_PROPERTY(QColor outline MEMBER m_outline NOTIFY changed)
    Q_PROPERTY(QColor outlineVariant MEMBER m_outlineVariant NOTIFY changed)
    Q_PROPERTY(QColor primary MEMBER m_primary NOTIFY changed)
    Q_PROPERTY(QColor primaryContainer MEMBER m_primaryContainer NOTIFY changed)
    Q_PROPERTY(QColor primaryFixed MEMBER m_primaryFixed NOTIFY changed)
    Q_PROPERTY(QColor primaryFixedDim MEMBER m_primaryFixedDim NOTIFY changed)
    Q_PROPERTY(QColor scrim MEMBER m_scrim NOTIFY changed)
    Q_PROPERTY(QColor secondary MEMBER m_secondary NOTIFY changed)
    Q_PROPERTY(QColor secondaryContainer MEMBER m_secondaryContainer NOTIFY changed)
    Q_PROPERTY(QColor secondaryFixed MEMBER m_secondaryFixed NOTIFY changed)
    Q_PROPERTY(QColor secondaryFixedDim MEMBER m_secondaryFixedDim NOTIFY changed)
    Q_PROPERTY(QColor shadow MEMBER m_shadow NOTIFY changed)
    Q_PROPERTY(QColor surface MEMBER m_surface NOTIFY changed)
    Q_PROPERTY(QColor surfaceBright MEMBER m_surfaceBright NOTIFY changed)
    Q_PROPERTY(QColor surfaceContainer MEMBER m_surfaceContainer NOTIFY changed)
    Q_PROPERTY(QColor surfaceContainerHigh MEMBER m_surfaceContainerHigh NOTIFY changed)
    Q_PROPERTY(QColor surfaceContainerHighest MEMBER m_surfaceContainerHighest NOTIFY changed)
    Q_PROPERTY(QColor surfaceContainerLow MEMBER m_surfaceContainerLow NOTIFY changed)
    Q_PROPERTY(QColor surfaceContainerLowest MEMBER m_surfaceContainerLowest NOTIFY changed)
    Q_PROPERTY(QColor surfaceDim MEMBER m_surfaceDim NOTIFY changed)
    Q_PROPERTY(QColor surfaceTint MEMBER m_surfaceTint NOTIFY changed)
    Q_PROPERTY(QColor surfaceVariant MEMBER m_surfaceVariant NOTIFY changed)
    Q_PROPERTY(QColor tertiary MEMBER m_tertiary NOTIFY changed)
    Q_PROPERTY(QColor tertiaryContainer MEMBER m_tertiaryContainer NOTIFY changed)
    Q_PROPERTY(QColor tertiaryFixed MEMBER m_tertiaryFixed NOTIFY changed)
    Q_PROPERTY(QColor tertiaryFixedDim MEMBER m_tertiaryFixedDim NOTIFY changed)

public:
    explicit Palette(QObject *parent = nullptr);

    bool darkMode() const;

Q_SIGNALS:
    void changed();

private:
    void reload();

    QColor m_background;
    QColor m_error;
    QColor m_errorContainer;
    QColor m_inverseOnSurface;
    QColor m_inversePrimary;
    QColor m_inverseSurface;
    QColor m_onBackground;
    QColor m_onError;
    QColor m_onErrorContainer;
    QColor m_onPrimary;
    QColor m_onPrimaryContainer;
    QColor m_onPrimaryFixed;
    QColor m_onPrimaryFixedVariant;
    QColor m_onSecondary;
    QColor m_onSecondaryContainer;
    QColor m_onSecondaryFixed;
    QColor m_onSecondaryFixedVariant;
    QColor m_onSurface;
    QColor m_onSurfaceVariant;
    QColor m_onTertiary;
    QColor m_onTertiaryContainer;
    QColor m_onTertiaryFixed;
    QColor m_onTertiaryFixedVariant;
    QColor m_outline;
    QColor m_outlineVariant;
    QColor m_primary;
    QColor m_primaryContainer;
    QColor m_primaryFixed;
    QColor m_primaryFixedDim;
    QColor m_scrim;
    QColor m_secondary;
    QColor m_secondaryContainer;
    QColor m_secondaryFixed;
    QColor m_secondaryFixedDim;
    QColor m_shadow;
    QColor m_surface;
    QColor m_surfaceBright;
    QColor m_surfaceContainer;
    QColor m_surfaceContainerHigh;
    QColor m_surfaceContainerHighest;
    QColor m_surfaceContainerLow;
    QColor m_surfaceContainerLowest;
    QColor m_surfaceDim;
    QColor m_surfaceTint;
    QColor m_surfaceVariant;
    QColor m_tertiary;
    QColor m_tertiaryContainer;
    QColor m_tertiaryFixed;
    QColor m_tertiaryFixedDim;
};

}
