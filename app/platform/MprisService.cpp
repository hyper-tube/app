#include "MprisService.h"

#include "MprisPlayerAdaptor.h"
#include "MprisRootAdaptor.h"
#include "core/AppInfo.h"
#include "core/Logging.h"
#include "media/PlaybackController.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QVariantMap>

namespace {

const QString kObjectPath = QStringLiteral("/org/mpris/MediaPlayer2");
const QString kPlayerInterface = QStringLiteral("org.mpris.MediaPlayer2.Player");
const QString kPropertiesInterface = QStringLiteral("org.freedesktop.DBus.Properties");
const QString kPropertiesChanged = QStringLiteral("PropertiesChanged");

const QStringList kTrackProperties {
    QStringLiteral("Metadata"),
    QStringLiteral("CanPlay"),
    QStringLiteral("CanPause"),
    QStringLiteral("CanSeek"),
};

}

namespace platform {

MprisService::MprisService(media::PlaybackController &controller, QObject *parent)
    : MediaControls(controller, parent)
    , m_player(new MprisPlayerAdaptor(this))
{
    new MprisRootAdaptor(this);

    m_flushTimer.setSingleShot(true);
    m_flushTimer.setInterval(0);
    connect(&m_flushTimer, &QTimer::timeout, this, &MprisService::flush);

    using Controller = media::PlaybackController;
    connect(&controller, &Controller::trackChanged, this, [this] { publish(kTrackProperties); });
    connect(&controller, &Controller::durationChanged, this, [this] { publish(kTrackProperties); });
    connect(&controller, &Controller::playingChanged, this,
            [this] { publish({QStringLiteral("PlaybackStatus")}); });
    connect(&controller, &Controller::reachChanged, this,
            [this] { publish({QStringLiteral("CanGoNext"), QStringLiteral("CanGoPrevious")}); });
    connect(&controller, &Controller::shuffleChanged, this,
            [this] { publish({QStringLiteral("Shuffle")}); });
    connect(&controller, &Controller::repeatChanged, this,
            [this] { publish({QStringLiteral("LoopStatus")}); });
    connect(&controller, &Controller::volumeChanged, this,
            [this] { publish({QStringLiteral("Volume")}); });
    connect(&controller, &Controller::mutedChanged, this,
            [this] { publish({QStringLiteral("Volume")}); });
    connect(&controller, &Controller::seeked, this,
            [this](qint64 milliseconds) { m_player->reportSeek(milliseconds * 1000); });

    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        qCWarning(logPlatform) << "no session bus, media controls are unavailable";
        return;
    }
    if (!bus.registerObject(kObjectPath, this)) {
        qCWarning(logPlatform) << "cannot export" << kObjectPath;
        return;
    }
    const QString serviceName =
        QStringLiteral("org.mpris.MediaPlayer2.") + core::AppInfo::identifier();
    if (!bus.registerService(serviceName)) {
        qCWarning(logPlatform) << "cannot own" << serviceName;
        return;
    }
    qCInfo(logPlatform) << "media controls published as" << serviceName;
}

void MprisService::publish(const QStringList &properties)
{
    for (const QString &property : properties) {
        if (!m_dirty.contains(property))
            m_dirty.append(property);
    }
    m_flushTimer.start();
}

void MprisService::flush()
{
    QVariantMap changed;
    for (const QString &property : std::as_const(m_dirty))
        changed.insert(property, m_player->property(property.toLatin1().constData()));
    m_dirty.clear();
    if (changed.isEmpty())
        return;

    QDBusMessage message =
        QDBusMessage::createSignal(kObjectPath, kPropertiesInterface, kPropertiesChanged);
    message << kPlayerInterface << changed << QStringList();
    QDBusConnection::sessionBus().send(message);
}

}
