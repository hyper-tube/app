#include "Account.h"

#include "WebLogin.h"
#include "core/Logging.h"
#include "innertube/parsers/RendererReader.h"
#include "net/Connectivity.h"
#include "net/CookieJar.h"
#include "net/HttpClient.h"

#include <QCoreApplication>
#include <QJsonObject>
#include <QQmlEngine>
#include <QSettings>

namespace {

constexpr int kRenewalTimeoutMs = 45000;
constexpr int kRenewalCooldownMs = 300000;

const QString kAccountGroup = QStringLiteral("account");
const QString kNameKey = QStringLiteral("account/name");
const QString kHandleKey = QStringLiteral("account/handle");
const QString kArtKey = QStringLiteral("account/artId");
const QString kChannelKey = QStringLiteral("account/channelId");
const QString kIdentityKey = QStringLiteral("account/dataSyncId");

QString identityFrom(const QJsonObject &response)
{
    const QJsonObject context = response.value(QStringLiteral("responseContext"))
                                    .toObject()
                                    .value(QStringLiteral("mainAppWebResponseContext"))
                                    .toObject();
    QString id = context.value(QStringLiteral("datasyncId")).toString();
    if (id.isEmpty())
        id = context.value(QStringLiteral("dataSyncId")).toString();
    return id;
}

bool connected()
{
    return net::Connectivity::instance().online();
}

}

namespace auth {

Account::Account(QObject *parent)
    : QObject(parent)
    , m_endpoints(innertube::Session::instance())
{
    m_renewalTimeout.setSingleShot(true);
    m_renewalTimeout.setInterval(kRenewalTimeoutMs);
    connect(&m_renewalTimeout, &QTimer::timeout, this, [this] {
        endRenewal();
        if (!connected()) {
            qCInfo(logInnerTube) << "session renewal interrupted by the network";
            m_renewalCooldown = QDeadlineTimer();
            m_unverified = true;
            return;
        }
        qCWarning(logInnerTube) << "session renewal timed out";
        expire();
    });

    connect(&WebLogin::instance(), &WebLogin::harvested, this, [this](bool credentialed) {
        const bool wasRenewing = m_renewing;
        endRenewal();
        if (credentialed && wasRenewing)
            verify();
        else if (credentialed)
            refresh();
        else if (wasRenewing)
            expire();
        else
            setStatus(SignedOut, tr("That sign-in did not produce a credential. Try again."));
    });

    connect(&innertube::Session::instance(), &innertube::Session::rejected, this, &Account::renew);
    connect(&net::Connectivity::instance(), &net::Connectivity::onlineChanged, this, [this] {
        if (m_unverified && connected())
            verify();
    });

    if (innertube::Session::instance().authenticated())
        restore();
    else if (QSettings().contains(kNameKey))
        expire();
}

Account &Account::instance()
{
    static auto *account = new Account(QCoreApplication::instance());
    return *account;
}

Account *Account::create(QQmlEngine *, QJSEngine *)
{
    QQmlEngine::setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
    return &instance();
}

void Account::refresh()
{
    if (!innertube::Session::instance().authenticated()) {
        setStatus(SignedOut);
        return;
    }
    setStatus(SigningIn);
    verify();
}

void Account::restore()
{
    const QSettings settings;
    if (!settings.contains(kNameKey)) {
        refresh();
        return;
    }
    m_name = settings.value(kNameKey).toString();
    m_handle = settings.value(kHandleKey).toString();
    m_artId = settings.value(kArtKey).toString();
    m_channelId = settings.value(kChannelKey).toString();
    innertube::Session::instance().setIdentity(settings.value(kIdentityKey).toString());
    qCInfo(logInnerTube) << "restored the account of" << m_name;
    setStatus(SignedIn);
    verify();
}

void Account::verify()
{
    m_unverified = false;
    if (!innertube::Session::instance().authenticated())
        return;
    m_endpoints.accountMenu([this](const innertube::Reply &reply) { accept(reply); });
}

void Account::signOut()
{
    endRenewal();
    m_unverified = false;
    net::HttpClient::instance().cookies().discard();
    WebLogin::instance().forget();
    innertube::Session::instance().setIdentity({});
    QSettings().remove(kAccountGroup);
    m_name.clear();
    m_handle.clear();
    m_artId.clear();
    m_channelId.clear();
    setStatus(SignedOut);
}

void Account::requestSignIn()
{
    if (connected())
        Q_EMIT signInRequested();
    else
        Q_EMIT signInUnavailable();
}

void Account::accept(const innertube::Reply &reply)
{
    using innertube::parsers::findFirst;
    using innertube::parsers::readText;
    using innertube::parsers::readThumbnail;

    if (!innertube::Session::instance().authenticated())
        return;

    if (reply.unreachable) {
        qCInfo(logInnerTube) << "account check waits for the network";
        m_unverified = true;
        if (m_status == SigningIn)
            setStatus(SignedIn);
        return;
    }

    if (!reply.ok()) {
        qCWarning(logInnerTube) << "account menu failed" << reply.error;
        if (m_status == SigningIn && !m_renewing)
            expire();
        return;
    }

    const QJsonObject header =
        findFirst(reply.json, QStringLiteral("activeAccountHeaderRenderer")).toObject();
    if (header.isEmpty()) {
        qCWarning(logInnerTube) << "account menu carried no active account";
        expire();
        return;
    }

    m_name = readText(header.value(QStringLiteral("accountName")));
    m_handle = readText(header.value(QStringLiteral("channelHandle")));
    m_artId = readThumbnail(header.value(QStringLiteral("accountPhoto")));
    m_channelId = findFirst(header, QStringLiteral("browseEndpoint"))
                      .toObject()
                      .value(QStringLiteral("browseId"))
                      .toString();
    const QString identity = identityFrom(reply.json);
    innertube::Session::instance().setIdentity(identity);
    remember(identity);

    qCInfo(logInnerTube) << "signed in as" << m_name;
    const bool settled = m_status == SignedIn && m_error.isEmpty();
    setStatus(SignedIn);
    if (settled)
        Q_EMIT changed();
}

void Account::remember(const QString &identity) const
{
    QSettings settings;
    settings.setValue(kNameKey, m_name);
    settings.setValue(kHandleKey, m_handle);
    settings.setValue(kArtKey, m_artId);
    settings.setValue(kChannelKey, m_channelId);
    settings.setValue(kIdentityKey, identity);
}

void Account::renew()
{
    if ((m_status != SignedIn && m_status != SigningIn) || m_renewing
        || !m_renewalCooldown.hasExpired())
        return;
    m_renewing = true;
    m_renewalCooldown = QDeadlineTimer(kRenewalCooldownMs);
    m_renewalTimeout.start();
    qCInfo(logInnerTube) << "renewing the session from the login profile";
    Q_EMIT changed();
}

void Account::endRenewal()
{
    m_renewalTimeout.stop();
    if (!m_renewing)
        return;
    m_renewing = false;
    Q_EMIT changed();
}

void Account::expire()
{
    setStatus(Expired, tr("Your YouTube Music session expired. Sign in again."));
}

void Account::setStatus(Status status, const QString &error)
{
    if (m_status == status && m_error == error)
        return;
    const bool raised = !error.isEmpty() && error != m_error;
    m_status = status;
    m_error = error;
    Q_EMIT changed();
    if (raised)
        Q_EMIT errorRaised();
}

}
