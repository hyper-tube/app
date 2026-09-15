#pragma once

#include <QObject>

class QPointerEvent;
class QQuickWindow;

namespace diagnostics {

class StateObserver;

class InteractionTracker : public QObject
{
    Q_OBJECT

public:
    explicit InteractionTracker(const StateObserver &state, QObject *parent = nullptr);

    void setEnabled(bool enabled);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void record(const QPointerEvent &event) const;

    const StateObserver &m_state;
    bool m_enabled = false;
};

}
