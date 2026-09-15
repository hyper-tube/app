#include "FirefoxAgent.h"

#include "core/Logging.h"
#include "net/HttpClient.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QSettings>
#include <QUrl>

namespace {

#if defined(Q_OS_WIN)
const QString kPlatform = QStringLiteral("Windows NT 10.0; Win64; x64");
#elif defined(Q_OS_MACOS)
const QString kPlatform = QStringLiteral("Macintosh; Intel Mac OS X 10.15");
#else
const QString kPlatform = QStringLiteral("X11; Linux x86_64");
#endif

const QString kBuiltInVersion = QStringLiteral("155.0");
const QString kReleasesUrl =
    QStringLiteral("https://product-details.mozilla.org/1.0/firefox_versions.json");
const QString kReleaseField = QStringLiteral("LATEST_FIREFOX_VERSION");

constexpr auto kVersionKey = "login/firefoxVersion";
constexpr auto kCheckedKey = "login/firefoxVersionChecked";
constexpr int kFreshForDays = 7;

QString majorVersion(const QString &release)
{
    bool numeric = false;
    const int major = release.section(QLatin1Char('.'), 0, 0).toInt(&numeric);
    if (!numeric || major <= 0)
        return {};
    return QString::number(major) + QStringLiteral(".0");
}

}

namespace auth {

FirefoxAgent::FirefoxAgent(QObject *parent)
    : QObject(parent)
    , m_version(QSettings().value(kVersionKey, kBuiltInVersion).toString())
{
    if (majorVersion(m_version).isEmpty())
        m_version = kBuiltInVersion;
}

QString FirefoxAgent::userAgent() const
{
    return QStringLiteral("Mozilla/5.0 (") + kPlatform + QStringLiteral("; rv:") + m_version
        + QStringLiteral(") Gecko/20100101 Firefox/") + m_version;
}

void FirefoxAgent::refresh()
{
    const QDateTime checked = QSettings().value(kCheckedKey).toDateTime();
    if (checked.isValid() && checked.daysTo(QDateTime::currentDateTimeUtc()) < kFreshForDays)
        return;

    const QPointer<FirefoxAgent> guard(this);
    net::HttpClient::instance().get(QUrl(kReleasesUrl), {}, net::Credentialed::No,
                                    [this, guard](const net::Response &response) {
        if (!guard || !response.ok())
            return;
        adopt(majorVersion(
            QJsonDocument::fromJson(response.body).object().value(kReleaseField).toString()));
    }, true);
}

void FirefoxAgent::adopt(const QString &version)
{
    if (version.isEmpty())
        return;

    QSettings settings;
    settings.setValue(kCheckedKey, QDateTime::currentDateTimeUtc());
    if (m_version == version)
        return;

    m_version = version;
    settings.setValue(kVersionKey, version);
    qCInfo(logNet) << "sign-in now presents as Firefox" << version;
    Q_EMIT changed();
}

}
