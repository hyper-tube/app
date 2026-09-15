#include "InteractionTracker.h"

#include "Diagnostics.h"
#include "StateObserver.h"

#include <QCoreApplication>
#include <QList>
#include <QMouseEvent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>

namespace {

constexpr int kMaximumDepth = 64;
constexpr qsizetype kPathLabels = 4;
constexpr qsizetype kPathLength = 64;
constexpr QByteArrayView kComponentMarker("_QMLTYPE_");
constexpr QLatin1StringView kRootId("root");

struct Target
{
    QList<QByteArray> labels;
    QByteArray type;
    QByteArray surface = QByteArrayLiteral("window");

    QByteArray control() const { return labels.isEmpty() ? QByteArray() : labels.constFirst(); }

    QByteArray path() const
    {
        QByteArray path;
        for (const QByteArray &label : labels) {
            const QByteArray extended = path.isEmpty() ? label : label + '/' + path;
            if (extended.size() > kPathLength)
                break;
            path = extended;
        }
        return path;
    }
};

QByteArray componentName(const QObject *object)
{
    const QByteArrayView className(object->metaObject()->className());
    const qsizetype marker = className.indexOf(kComponentMarker);
    return marker > 0 ? className.first(marker).toByteArray() : QByteArray();
}

QByteArray declaredIdentifier(const QObject *object)
{
    if (!object->objectName().isEmpty())
        return object->objectName().toLatin1();
    for (const QQmlContext *context = qmlContext(object); context;
         context = context->parentContext()) {
        const QString id = context->nameForObject(object);
        if (!id.isEmpty() && id != kRootId)
            return id.toLatin1();
    }
    return {};
}

QByteArray identifierOf(const QObject *object)
{
    const QByteArray identifier = declaredIdentifier(object);
    return diagnostics::Value::symbolic(QLatin1StringView(identifier)) ? identifier : QByteArray();
}

QObject *containerOf(const QObject *object)
{
    if (const auto *item = qobject_cast<const QQuickItem *>(object)) {
        QQuickItem *parent = item->parentItem();
        if (parent && !parent->inherits("QQuickOverlay"))
            return parent;
    }
    return object->parent();
}

QQuickItem *itemOf(QObject *object)
{
    while (object && !object->isWindowType()) {
        if (auto *item = qobject_cast<QQuickItem *>(object))
            return item;
        object = object->parent();
    }
    return nullptr;
}

Target targetOf(QQuickItem *control)
{
    Target target;
    QObject *object = control;
    for (int depth = 0; object && !object->isWindowType() && depth < kMaximumDepth;
         ++depth, object = containerOf(object)) {
        const QByteArray component = componentName(object);
        const QByteArray identifier = identifierOf(object);
        if (component.isEmpty() && identifier.isEmpty())
            continue;
        if (target.labels.isEmpty())
            target.type = component;
        else if (!component.isEmpty())
            target.surface = component;
        if (target.labels.size() < kPathLabels)
            target.labels.append(identifier.isEmpty() ? component : identifier);
    }
    return target;
}

QObject *grabberOf(const QPointerEvent &event, const QEventPoint &point)
{
    if (QObject *exclusive = event.exclusiveGrabber(point))
        return exclusive;
    const QList<QPointer<QObject>> passive = event.passiveGrabbers(point);
    return passive.isEmpty() ? nullptr : passive.constFirst().data();
}

const char *buttonName(Qt::MouseButton button)
{
    switch (button) {
    case Qt::RightButton: return "right";
    case Qt::MiddleButton: return "middle";
    default: break;
    }
    return "left";
}

diagnostics::Value symbolOf(const QByteArray &text)
{
    return diagnostics::Value::symbol(QLatin1StringView(text));
}

}

namespace diagnostics {

InteractionTracker::InteractionTracker(const StateObserver &state, QObject *parent)
    : QObject(parent)
    , m_state(state)
{
}

void InteractionTracker::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;
    m_enabled = enabled;
    if (enabled)
        QCoreApplication::instance()->installEventFilter(this);
    else
        QCoreApplication::instance()->removeEventFilter(this);
}

bool InteractionTracker::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonRelease && watched->isWindowType()
        && qobject_cast<QQuickWindow *>(watched))
        record(static_cast<const QPointerEvent &>(*event));
    return QObject::eventFilter(watched, event);
}

void InteractionTracker::record(const QPointerEvent &event) const
{
    if (event.pointCount() != 1)
        return;
    const QEventPoint &point = event.points().constFirst();
    QQuickItem *item = itemOf(grabberOf(event, point));
    if (!item || item->inherits("QQuickFlickable")
        || !item->contains(item->mapFromScene(point.scenePosition())))
        return;

    const Target target = targetOf(item);
    if (target.labels.isEmpty())
        return;

    const auto &mouse = static_cast<const QMouseEvent &>(event);
    breadcrumb("ui.activate",
               {{"control", symbolOf(target.control())},
                {"type", symbolOf(target.type)},
                {"path", symbolOf(target.path())},
                {"surface", symbolOf(target.surface)},
                {"page", symbolOf(m_state.page())},
                {"button", buttonName(mouse.button())}});
}

}
