#pragma once

#include "InteractionTracker.h"
#include "StateObserver.h"

#include <QObject>
#include <QQmlEngine>

namespace diagnostics {

class DiagnosticsSettings : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool available READ available CONSTANT)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)

public:
    explicit DiagnosticsSettings(QObject *parent);

    static DiagnosticsSettings &instance();
    static DiagnosticsSettings *create(QQmlEngine *, QJSEngine *);

    bool available() const;
    bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled);

    void restore();
    void bind(QQmlEngine &engine);

Q_SIGNALS:
    void enabledChanged();

private:
    void activate();
    void deactivate();

    StateObserver m_state;
    InteractionTracker m_interactions;
    bool m_enabled = false;
};

}
