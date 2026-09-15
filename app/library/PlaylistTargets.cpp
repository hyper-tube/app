#include "PlaylistTargets.h"

#include "LibraryActions.h"
#include "auth/Account.h"
#include "core/Logging.h"
#include "innertube/parsers/RendererReader.h"
#include "net/Connectivity.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QPointer>
#include <QQmlEngine>

namespace {

enum Role {
    Title = Qt::UserRole,
    Subtitle,
    ArtId,
};

const QString kEndpoint = QStringLiteral("playlist/get_add_to_playlist");

}

namespace library {

PlaylistTargets::PlaylistTargets(QObject *parent)
    : QAbstractListModel(parent)
    , m_endpoints(innertube::Session::instance())
{
    connect(&LibraryActions::instance(), &LibraryActions::collectionChanged, this, [this] {
        if (m_open)
            reload();
    });
    connect(&net::Connectivity::instance(), &net::Connectivity::onlineChanged, this, [this] {
        if (m_open && m_unreachable && net::Connectivity::instance().online())
            reload();
    });
}

PlaylistTargets &PlaylistTargets::instance()
{
    static auto *targets = new PlaylistTargets(QCoreApplication::instance());
    return *targets;
}

PlaylistTargets *PlaylistTargets::create(QQmlEngine *, QJSEngine *)
{
    QQmlEngine::setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
    return &instance();
}

int PlaylistTargets::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_targets.size();
}

QVariant PlaylistTargets::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_targets.size())
        return {};
    const Target &target = m_targets.at(index.row());
    switch (role) {
    case Title: return target.title;
    case Subtitle: return target.subtitle;
    case ArtId: return target.artId;
    default: return {};
    }
}

QHash<int, QByteArray> PlaylistTargets::roleNames() const
{
    return {{Title, "targetTitle"}, {Subtitle, "targetSubtitle"}, {ArtId, "targetArtId"}};
}

void PlaylistTargets::show(const QStringList &videoIds)
{
    m_videoIds = videoIds;
    m_videoIds.removeAll(QString());
    if (m_videoIds.isEmpty())
        return;
    m_open = true;
    reload();
}

void PlaylistTargets::dismiss()
{
    if (!m_open)
        return;
    m_open = false;
    ++m_generation;
    m_loading = false;
    m_videoIds.clear();
    Q_EMIT stateChanged();
}

void PlaylistTargets::reload()
{
    if (m_videoIds.isEmpty() || !auth::Account::instance().signedIn())
        return;
    beginResetModel();
    m_targets.clear();
    endResetModel();
    m_error.clear();
    m_unreachable = false;
    m_loading = true;
    Q_EMIT stateChanged();

    QJsonArray videoIds;
    for (const QString &videoId : std::as_const(m_videoIds))
        videoIds.append(videoId);
    const quint64 generation = ++m_generation;
    const QPointer<PlaylistTargets> guard(this);
    m_endpoints.action(
        kEndpoint,
        {{QStringLiteral("videoIds"), videoIds}, {QStringLiteral("excludeWatchLater"), false}},
        [guard, generation](const innertube::Reply &reply) {
        if (guard)
            guard->accept(reply, generation);
    });
}

void PlaylistTargets::retry()
{
    if (m_unreachable && !net::Connectivity::instance().settled())
        net::Connectivity::instance().check();
    else
        reload();
}

void PlaylistTargets::accept(const innertube::Reply &reply, quint64 generation)
{
    if (generation != m_generation)
        return;
    m_loading = false;
    if (!reply.ok()) {
        m_unreachable = reply.unreachable;
        m_error = m_unreachable ? tr("Connect to the internet to see your playlists.")
                                : tr("Check your connection and try again.");
        qCWarning(logInnerTube) << "add to playlist failed" << reply.error;
        Q_EMIT stateChanged();
        return;
    }
    QList<Target> targets;
    const QJsonValue options =
        innertube::parsers::findFirst(reply.json, QStringLiteral("playlists"));
    for (const QJsonValue &value : options.toArray()) {
        const QJsonObject option =
            value.toObject().value(QStringLiteral("playlistAddToOptionRenderer")).toObject();
        Target target;
        target.playlistId = option.value(QStringLiteral("playlistId")).toString();
        target.title = innertube::parsers::readText(option.value(QStringLiteral("title")));
        target.subtitle =
            innertube::parsers::readText(option.value(QStringLiteral("shortBylineText")));
        target.artId =
            innertube::parsers::readThumbnail(option.value(QStringLiteral("thumbnailRenderer")));
        if (!target.playlistId.isEmpty() && !target.title.isEmpty())
            targets.append(target);
    }
    beginResetModel();
    m_targets = std::move(targets);
    endResetModel();
    Q_EMIT stateChanged();
}

void PlaylistTargets::choose(int index)
{
    if (index < 0 || index >= m_targets.size())
        return;
    const Target target = m_targets.at(index);
    LibraryActions::instance().addToPlaylist(target.playlistId, target.title, m_videoIds);
    dismiss();
}

void PlaylistTargets::createWith(const QString &title)
{
    if (title.trimmed().isEmpty() || m_videoIds.isEmpty())
        return;
    LibraryActions::instance().createPlaylistWith(title, m_videoIds);
    dismiss();
}

}
