#include "Credentials.h"

#include "net/CookieJar.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QStringList>

namespace {

struct SigningCookie
{
    QString scheme;
    QString name;
};

const QString kSapisid = QStringLiteral("SAPISID");
const QString kSapisidAlias = QStringLiteral("__Secure-3PAPISID");

const QList<SigningCookie> kSigningCookies {
    {QStringLiteral("SAPISIDHASH"), kSapisid},
    {QStringLiteral("SAPISID1PHASH"), QStringLiteral("__Secure-1PAPISID")},
    {QStringLiteral("SAPISID3PHASH"), kSapisidAlias},
};

QString signingValue(const net::CookieJar &jar, const QString &name)
{
    const QString value = jar.value(name);
    return !value.isEmpty() || name != kSapisid ? value : jar.value(kSapisidAlias);
}

QString signature(qint64 epoch, const QString &value, const QString &origin)
{
    const QString payload =
        QString::number(epoch) + QLatin1Char(' ') + value + QLatin1Char(' ') + origin;
    return QString::fromLatin1(
        QCryptographicHash::hash(payload.toUtf8(), QCryptographicHash::Sha1).toHex());
}

}

namespace auth::credentials {

bool present(const net::CookieJar &jar)
{
    return !signingValue(jar, kSapisid).isEmpty();
}

QByteArray authorization(const net::CookieJar &jar, const QString &origin)
{
    const qint64 epoch = QDateTime::currentSecsSinceEpoch();
    QStringList schemes;
    for (const SigningCookie &cookie : kSigningCookies) {
        const QString value = signingValue(jar, cookie.name);
        if (value.isEmpty())
            continue;
        schemes.append(cookie.scheme + QLatin1Char(' ') + QString::number(epoch) + QLatin1Char('_')
                       + signature(epoch, value, origin));
    }
    return schemes.join(QLatin1Char(' ')).toUtf8();
}

}
