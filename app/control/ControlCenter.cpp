#include "ControlCenter.h"

#include <QCoreApplication>
#include <QSettings>

#include <algorithm>
#include <utility>

namespace {

constexpr int kDismissedMemory = 200;

}

namespace control {

ControlCenter::ControlCenter(QObject *parent)
    : QAbstractListModel(parent)
{
    const QSettings settings;
    m_dismissed = settings.value(QStringLiteral("control/dismissed")).toStringList();
}

ControlCenter &ControlCenter::instance()
{
    static auto *center = new ControlCenter(QCoreApplication::instance());
    return *center;
}

ControlCenter *ControlCenter::create(QQmlEngine *, QJSEngine *)
{
    QQmlEngine::setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
    return &instance();
}

int ControlCenter::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant ControlCenter::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()
        || role != Qt::UserRole)
        return {};
    return QVariant::fromValue(m_entries.at(index.row()));
}

QHash<int, QByteArray> ControlCenter::roleNames() const
{
    return {{Qt::UserRole, "entry"}};
}

int ControlCenter::unreadCount() const
{
    return int(std::count_if(m_entries.cbegin(), m_entries.cend(),
                             [](const Notification &entry) { return entry.unread; }));
}

bool ControlCenter::unread(const QString &id) const
{
    const int row = indexOf(id);
    return row >= 0 && m_entries.at(row).unread;
}

bool ControlCenter::clearable() const
{
    return std::ranges::any_of(m_entries,
                               [](const Notification &entry) { return entry.dismissible(); });
}

ControlCenter::Status ControlCenter::status() const
{
    if (!m_entries.isEmpty() && m_entries.constFirst().activity)
        return Busy;
    return unreadCount() > 0 ? Alert : Quiet;
}

double ControlCenter::progress() const
{
    if (m_entries.isEmpty() || !m_entries.constFirst().activity)
        return 0;
    return m_entries.constFirst().progress;
}

int ControlCenter::indexOf(const QString &id) const
{
    for (int row = 0; row < m_entries.size(); ++row) {
        if (m_entries.at(row).id == id)
            return row;
    }
    return -1;
}

int ControlCenter::placeFor(const Notification &entry) const
{
    for (int row = 0; row < m_entries.size(); ++row) {
        const Notification &existing = m_entries.at(row);
        if (entry.activity && !existing.activity)
            return row;
        if (entry.activity == existing.activity && entry.postedAt > existing.postedAt)
            return row;
    }
    return m_entries.size();
}

void ControlCenter::publish(const Notification &entry)
{
    if (entry.id.isEmpty() || m_dismissed.contains(entry.id))
        return;
    Notification posted = entry;
    if (!posted.postedAt.isValid())
        posted.postedAt = QDateTime::currentDateTime();
    const int existing = indexOf(posted.id);
    if (existing >= 0) {
        const bool wasActivity = m_entries.at(existing).activity;
        posted.unread = m_entries.at(existing).unread;
        if (wasActivity == posted.activity) {
            m_entries[existing] = std::move(posted);
            Q_EMIT dataChanged(index(existing), index(existing));
            Q_EMIT progressChanged();
            return;
        }
        retract(posted.id);
    } else {
        posted.unread = posted.dismissible() && !posted.quiet;
    }
    const int row = placeFor(posted);
    beginInsertRows({}, row, row);
    m_entries.insert(row, posted);
    endInsertRows();
    Q_EMIT changed();
    Q_EMIT progressChanged();
    if (posted.unread)
        Q_EMIT arrived();
}

void ControlCenter::retract(const QString &id)
{
    const int row = indexOf(id);
    if (row < 0)
        return;
    beginRemoveRows({}, row, row);
    m_entries.remove(row);
    endRemoveRows();
    Q_EMIT changed();
    Q_EMIT progressChanged();
}

void ControlCenter::remember(const QString &id)
{
    m_dismissed.removeAll(id);
    m_dismissed.append(id);
    while (m_dismissed.size() > kDismissedMemory)
        m_dismissed.removeFirst();
    QSettings settings;
    settings.setValue(QStringLiteral("control/dismissed"), m_dismissed);
}

void ControlCenter::invokeAction(const QString &id)
{
    if (indexOf(id) < 0)
        return;
    Q_EMIT actionInvoked(id);
}

void ControlCenter::dismiss(const QString &id)
{
    const int row = indexOf(id);
    if (row < 0 || !m_entries.at(row).dismissible())
        return;
    remember(id);
    retract(id);
}

void ControlCenter::dismissAll()
{
    for (int row = m_entries.size() - 1; row >= 0; --row) {
        if (m_entries.at(row).dismissible())
            dismiss(m_entries.at(row).id);
    }
}

void ControlCenter::markRead()
{
    bool touched = false;
    for (int row = 0; row < m_entries.size(); ++row) {
        if (!m_entries.at(row).unread)
            continue;
        m_entries[row].unread = false;
        Q_EMIT dataChanged(index(row), index(row));
        touched = true;
    }
    if (touched)
        Q_EMIT changed();
}

}
