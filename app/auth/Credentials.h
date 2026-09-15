#pragma once

#include <QByteArray>
#include <QString>

namespace net {
class CookieJar;
}

namespace auth::credentials {

bool present(const net::CookieJar &jar);
QByteArray authorization(const net::CookieJar &jar, const QString &origin);

}
