#include "StateObserver.h"

#include "Diagnostics.h"
#include "auth/Account.h"
#include "core/Localization.h"
#include "innertube/Session.h"
#include "media/Browser.h"
#include "net/Connectivity.h"
#include "player/PlaybackSettings.h"
#include "plugin/PluginRegistry.h"

#include <QMetaEnum>
#include <QQmlEngine>

#include <algorithm>
#include <array>
#include <utility>

namespace {

constexpr std::array kPageKinds {
    QLatin1StringView("feed"),     QLatin1StringView("library"),   QLatin1StringView("search"),
    QLatin1StringView("history"),  QLatin1StringView("downloads"), QLatin1StringView("mix"),
    QLatin1StringView("playlist"), QLatin1StringView("album"),     QLatin1StringView("artist"),
    QLatin1StringView("podcast"),  QLatin1StringView("episode"),   QLatin1StringView("profile"),
    QLatin1StringView("mood"),     QLatin1StringView("song"),
};

constexpr std::array kServerSurfaceKinds {
    QLatin1StringView("feed"),
    QLatin1StringView("library"),
    QLatin1StringView("history"),
};

constexpr std::array kSections {"home", "explore", "library", "downloads"};

constexpr QLatin1StringView kServerSurfacePrefix("FEmusic_");

QByteArray pageOf(const model::BrowseModel *page)
{
    if (!page)
        return QByteArrayLiteral("none");
    const QString kind = page->kind();
    if (std::ranges::find(kPageKinds, kind) == kPageKinds.end())
        return QByteArrayLiteral("other");
    const QString &browseId = page->source().browseId;
    const bool serverSurface =
        std::ranges::find(kServerSurfaceKinds, kind) != kServerSurfaceKinds.end()
        && browseId.startsWith(kServerSurfacePrefix);
    return (serverSurface ? browseId : kind).toLatin1();
}

const char *sectionName(int section)
{
    return section >= 0 && std::cmp_less(section, kSections.size()) ? kSections.at(section)
                                                                    : "none";
}

const char *accountStatusName(int status)
{
    switch (status) {
    case auth::Account::SignedOut: return "signed_out";
    case auth::Account::SigningIn: return "signing_in";
    case auth::Account::SignedIn: return "signed_in";
    case auth::Account::Expired: return "expired";
    default: break;
    }
    return "unknown";
}

const char *transitionModeName(player::PlaybackSettings::TransitionMode mode)
{
    return QMetaEnum::fromType<player::PlaybackSettings::TransitionMode>().valueToKey(mode);
}

diagnostics::Value symbolOf(const QByteArray &text)
{
    return diagnostics::Value::symbol(QLatin1StringView(text));
}

}

namespace diagnostics {

StateObserver::StateObserver(QObject *parent)
    : QObject(parent)
{
}

void StateObserver::bind(QQmlEngine &engine)
{
    if (m_bound)
        return;
    m_bound = true;

    m_browser = engine.singletonInstance<media::Browser *>("HtMusic.App", "Browser");
    if (m_browser) {
        m_page = pageOf(m_browser->page());
        connect(m_browser, &media::Browser::pageChanged, this, &StateObserver::followPage);
    }

    m_accountStatus = auth::Account::instance().status();
    connect(&auth::Account::instance(), &auth::Account::changed, this,
            &StateObserver::followAccount);
    connect(&innertube::Session::instance(), &innertube::Session::rejected, this,
            [] { breadcrumb("auth.session_rejected", {}, Level::Warning); });

    connect(&net::Connectivity::instance(), &net::Connectivity::onlineChanged, this, [] {
        const bool online = net::Connectivity::instance().online();
        breadcrumb("network.connectivity_changed", {{"online", online}});
        setTag({"online", online});
    });

    connect(&core::Localization::instance(), &core::Localization::resolvedChanged, this,
            [] { setTag({"language", Value::symbol(core::Localization::instance().resolved())}); });

    const player::PlaybackSettings &settings = player::PlaybackSettings::instance();
    connect(&settings, &player::PlaybackSettings::transitionModeChanged, this,
            [this] { followSetting("transition_mode"); });
    connect(&settings, &player::PlaybackSettings::crossfadeChanged, this,
            [this] { followSetting("crossfade_seconds"); });
    connect(&settings, &player::PlaybackSettings::matchTempoChanged, this,
            [this] { followSetting("match_tempo"); });
    connect(&settings, &player::PlaybackSettings::equalizerChanged, this,
            [this] { followSetting("equalizer"); });
    connect(&settings, &player::PlaybackSettings::autoplayChanged, this,
            [this] { followSetting("autoplay"); });
    connect(&settings, &player::PlaybackSettings::playVideosChanged, this,
            [this] { followSetting("play_videos"); });

    for (const plugin::Plugin *entry : plugin::PluginRegistry::instance().plugins()) {
        connect(entry, &plugin::Plugin::enabledChanged, this, [this, entry] {
            breadcrumb(
                "plugin.toggled",
                {{"plugin", Value::symbol(entry->info().id)}, {"enabled", entry->enabled()}});
            publishPlugins();
        });
    }
}

void StateObserver::publish() const
{
    setTag({"page", symbolOf(m_page)});
    if (m_accountStatus >= 0)
        setTag({"account", accountStatusName(m_accountStatus)});
    setTag({"online", net::Connectivity::instance().online()});
    setTag({"language", Value::symbol(core::Localization::instance().resolved())});
    publishPlayback();
    publishPlugins();
}

void StateObserver::followPage()
{
    const QByteArray page = pageOf(m_browser ? m_browser->page() : nullptr);
    breadcrumb("navigation",
               {{"from", symbolOf(m_page)},
                {"to", symbolOf(page)},
                {"section", sectionName(m_browser ? m_browser->section() : -1)}});
    m_page = page;
    setTag({"page", symbolOf(m_page)});
}

void StateObserver::followAccount()
{
    const int status = auth::Account::instance().status();
    if (status == m_accountStatus)
        return;
    breadcrumb("auth.state_changed",
               {{"from", accountStatusName(m_accountStatus)}, {"to", accountStatusName(status)}});
    m_accountStatus = status;
    setTag({"account", accountStatusName(status)});
}

void StateObserver::followSetting(const char *setting) const
{
    breadcrumb("settings.changed", {{"setting", setting}});
    publishPlayback();
}

void StateObserver::publishPlayback() const
{
    const player::PlaybackSettings &settings = player::PlaybackSettings::instance();
    setContext("playback_settings",
               {{"transition_mode", transitionModeName(settings.transitionMode())},
                {"crossfade_seconds", settings.crossfadeSeconds()},
                {"match_tempo", settings.matchTempo()},
                {"equalizer", settings.equalizerEnabled()},
                {"normalize_loudness", settings.normalizeLoudness()},
                {"autoplay", settings.autoplay()},
                {"play_videos", settings.playVideos()}});
}

void StateObserver::publishPlugins() const
{
    for (const plugin::Plugin *entry : plugin::PluginRegistry::instance().plugins()) {
        const QByteArray key = QByteArrayLiteral("plugin.") + entry->info().id.toLatin1();
        setTag({key.constData(), entry->enabled()});
    }
}

}
