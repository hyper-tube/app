#include "WebLogin.h"

#include "Credentials.h"
#include "core/Logging.h"
#include "core/Paths.h"
#include "net/CookieJar.h"
#include "net/HttpClient.h"

#include <QCoreApplication>
#include <QQmlEngine>
#include <QQuickWebEngineProfile>
#include <QWebEngineClientHints>
#include <QWebEngineCookieStore>

namespace {

const QString kStorageName = QStringLiteral("login");
const QString kSignInUrl = QStringLiteral(
    "https://accounts.google.com/ServiceLogin?service=youtube&continue=https://music.youtube.com/");
const QString kLandingUrl = QStringLiteral("https://music.youtube.com/");
const QString kLandingHost = QStringLiteral("music.youtube.com");
const QString kAccountsHost = QStringLiteral("accounts.google.com");
const QString kRejectedPath = QStringLiteral("/signin/rejected");
const QString kYoutubeDomain = QStringLiteral("youtube.com");

constexpr int kPollIntervalMs = 400;
constexpr int kPollAttempts = 10;

const QString kAcceptLanguage = QStringLiteral("en-US,en;q=0.9");

bool belongsToYoutube(const QNetworkCookie &cookie)
{
    const QString domain =
        cookie.domain().startsWith(QLatin1Char('.')) ? cookie.domain().mid(1) : cookie.domain();
    return domain == kYoutubeDomain || domain.endsWith(QLatin1Char('.') + kYoutubeDomain);
}

QString identityOf(const QNetworkCookie &cookie)
{
    return QString::fromUtf8(cookie.name()) + QLatin1Char('\n') + cookie.domain()
        + QLatin1Char('\n') + cookie.path();
}

}

namespace auth {

WebLogin::WebLogin(QObject *parent)
    : QObject(parent)
    , m_agent(new FirefoxAgent(this))
    , m_profile(new QQuickWebEngineProfile(kStorageName, this))
{
    m_profile->setPersistentStoragePath(core::paths::dataDir() + QLatin1Char('/') + kStorageName);
    m_profile->setCachePath(core::paths::cacheDir() + QLatin1Char('/') + kStorageName);
    m_profile->setPersistentCookiesPolicy(QQuickWebEngineProfile::ForcePersistentCookies);
    m_profile->setHttpAcceptLanguage(kAcceptLanguage);
    m_profile->clientHints()->setAllClientHintsEnabled(false);
    applyAgent();

    connect(m_agent, &FirefoxAgent::changed, this, &WebLogin::applyAgent);
    m_agent->refresh();

    QWebEngineCookieStore const *store = m_profile->cookieStore();
    connect(store, &QWebEngineCookieStore::cookieAdded, this, [this](const QNetworkCookie &cookie) {
        if (belongsToYoutube(cookie))
            m_seen.insert(identityOf(cookie), cookie);
    });
    connect(store, &QWebEngineCookieStore::cookieRemoved, this,
            [this](const QNetworkCookie &cookie) { m_seen.remove(identityOf(cookie)); });

    m_poll.setInterval(kPollIntervalMs);
    connect(&m_poll, &QTimer::timeout, this, &WebLogin::poll);
}

WebLogin &WebLogin::instance()
{
    static auto *login = new WebLogin(QCoreApplication::instance());
    return *login;
}

WebLogin *WebLogin::create(QQmlEngine *, QJSEngine *)
{
    QQmlEngine::setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
    return &instance();
}

QUrl WebLogin::signInUrl() const
{
    return QUrl(kSignInUrl);
}

QUrl WebLogin::landingUrl() const
{
    return QUrl(kLandingUrl);
}

void WebLogin::follow(const QUrl &url)
{
    if (url.host() == kLandingHost) {
        harvest();
        return;
    }
    if (url.host() == kAccountsHost && url.path().endsWith(kRejectedPath)) {
        qCWarning(logNet) << "Google rejected the sign-in browser, the login profile starts over";
        forget();
    }
}

void WebLogin::harvest()
{
    if (m_poll.isActive())
        return;
    m_attempts = 0;
    m_profile->cookieStore()->loadAllCookies();
    m_poll.start();
    Q_EMIT harvestingChanged();
}

void WebLogin::forget()
{
    m_seen.clear();
    m_profile->cookieStore()->deleteAllCookies();
}

void WebLogin::applyAgent()
{
    m_profile->setHttpUserAgent(m_agent->userAgent());
    qCInfo(logNet) << "sign-in presents as" << m_profile->httpUserAgent();
}

void WebLogin::poll()
{
    ++m_attempts;
    net::CookieJar &jar = net::HttpClient::instance().cookies();
    const QList<QNetworkCookie> cookies = youtubeCookies();

    if (!cookies.isEmpty()) {
        jar.adopt(cookies);
        if (credentials::present(jar)) {
            m_poll.stop();
            qCInfo(logNet) << "adopted" << cookies.size() << "cookies from the login profile";
            Q_EMIT harvestingChanged();
            Q_EMIT harvested(true);
            return;
        }
    }

    if (m_attempts < kPollAttempts)
        return;

    m_poll.stop();
    qCWarning(logNet) << "the login profile never produced a credential";
    Q_EMIT harvestingChanged();
    Q_EMIT harvested(false);
}

QList<QNetworkCookie> WebLogin::youtubeCookies() const
{
    return m_seen.values();
}

}
