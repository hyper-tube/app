#include "CookieJar.h"

#include "core/Logging.h"
#include "core/Paths.h"

#include <QDateTime>
#include <QFile>
#include <QNetworkCookie>

namespace {

constexpr int kSaveDelayMs = 2000;

bool isDeletion(const QNetworkCookie &cookie)
{
    return !cookie.isSessionCookie() && cookie.expirationDate() <= QDateTime::currentDateTimeUtc();
}

QString identityOf(const QNetworkCookie &cookie)
{
    return QString::fromUtf8(cookie.name()) + QLatin1Char('\n') + cookie.domain()
        + QLatin1Char('\n') + cookie.path();
}

}

namespace net {

CookieJar::CookieJar(QObject *parent)
    : QNetworkCookieJar(parent)
    , m_path(core::paths::dataDir() + QStringLiteral("/cookies.txt"))
{
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(kSaveDelayMs);
    connect(&m_saveTimer, &QTimer::timeout, this, &CookieJar::save);
    load();
}

CookieJar::~CookieJar()
{
    if (m_saveTimer.isActive())
        save();
}

bool CookieJar::setCookiesFromUrl(const QList<QNetworkCookie> &cookies, const QUrl &url)
{
    QList<QNetworkCookie> kept;
    kept.reserve(cookies.size());
    for (const QNetworkCookie &cookie : cookies) {
        if (!isDeletion(cookie))
            kept.append(cookie);
    }

    if (kept.isEmpty())
        return false;

    const bool stored = QNetworkCookieJar::setCookiesFromUrl(kept, url);
    if (stored) {
        m_saveTimer.start();
        Q_EMIT changed();
    }
    return stored;
}

QString CookieJar::value(const QString &name) const
{
    const QByteArray wanted = name.toUtf8();
    QString found;
    int specificity = -1;
    for (const QNetworkCookie &cookie : allCookies()) {
        if (cookie.name() != wanted || cookie.domain().length() <= specificity)
            continue;
        specificity = cookie.domain().length();
        found = QString::fromUtf8(cookie.value());
    }
    return found;
}

bool CookieJar::adopt(const QList<QNetworkCookie> &cookies)
{
    QList<QNetworkCookie> merged = allCookies();
    QHash<QString, qsizetype> positions;
    for (qsizetype i = 0; i < merged.size(); ++i)
        positions.insert(identityOf(merged.at(i)), i);

    bool changedAny = false;
    for (const QNetworkCookie &cookie : cookies) {
        if (isDeletion(cookie))
            continue;
        const auto position = positions.constFind(identityOf(cookie));
        if (position == positions.constEnd()) {
            positions.insert(identityOf(cookie), merged.size());
            merged.append(cookie);
            changedAny = true;
        } else if (merged.at(*position).value() != cookie.value()) {
            merged[*position] = cookie;
            changedAny = true;
        }
    }

    if (!changedAny)
        return false;

    setAllCookies(merged);
    save();
    Q_EMIT changed();
    return true;
}

void CookieJar::discard()
{
    if (allCookies().isEmpty())
        return;
    setAllCookies({});
    save();
    Q_EMIT changed();
}

void CookieJar::load()
{
    QFile file(m_path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QList<QNetworkCookie> cookies;
    while (!file.atEnd()) {
        for (const QNetworkCookie &cookie :
             QNetworkCookie::parseCookies(file.readLine().trimmed())) {
            if (!isDeletion(cookie))
                cookies.append(cookie);
        }
    }

    setAllCookies(cookies);
    qCDebug(logNet) << "restored" << cookies.size() << "cookies";
}

void CookieJar::save()
{
    QFile file(m_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        qCWarning(logNet) << "cannot persist cookies to" << m_path;
        return;
    }
    file.setPermissions(QFile::ReadOwner | QFile::WriteOwner);

    for (const QNetworkCookie &cookie : allCookies()) {
        if (cookie.isSessionCookie())
            continue;
        file.write(cookie.toRawForm(QNetworkCookie::Full));
        file.write("\n");
    }
}

}
