#include "Release.h"

#include "core/Json.h"
#include "core/Logging.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

namespace {

constexpr int kSchema = 1;

#if defined(Q_PROCESSOR_ARM_64)
constexpr QLatin1StringView kArchitecture("arm64");
#else
constexpr QLatin1StringView kArchitecture("x64");
#endif

bool secureUrl(const QUrl &url)
{
    return url.isValid() && url.scheme() == QLatin1String("https") && !url.host().isEmpty();
}

QByteArray digestFrom(const QJsonValue &value)
{
    static const QRegularExpression hex(QStringLiteral("^[0-9a-fA-F]{64}$"));
    const QString text = value.toString();
    return hex.match(text).hasMatch() ? text.toLower().toLatin1() : QByteArray();
}

update::Release::Package windowsInstallerFrom(const QJsonArray &assets)
{
    for (const QJsonValue &value : assets) {
        const QJsonObject asset = value.toObject();
        if (asset.value(QLatin1String("platform")).toString() != QLatin1String("windows")
            || asset.value(QLatin1String("format")).toString() != QLatin1String("exe")
            || asset.value(QLatin1String("arch")).toString() != kArchitecture) {
            continue;
        }
        const QUrl url(asset.value(QLatin1String("url")).toString());
        const QString fileName =
            QFileInfo(asset.value(QLatin1String("name")).toString()).fileName();
        if (!secureUrl(url) || !fileName.endsWith(QLatin1String(".exe"), Qt::CaseInsensitive))
            continue;
        return {url, fileName, digestFrom(asset.value(QLatin1String("sha256"))),
                core::json::toInt(asset.value(QLatin1String("size")))};
    }
    return {};
}

}

namespace update {

std::optional<Release> Release::fromJson(const QByteArray &body)
{
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(body, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        qCWarning(logNet) << "release feed is not a JSON object" << error.errorString();
        return std::nullopt;
    }

    const QJsonObject root = document.object();
    const qint64 schema = core::json::toInt(root.value(QLatin1String("schema")));
    if (schema != kSchema) {
        qCWarning(logNet) << "release feed schema" << schema << "is not understood";
        return std::nullopt;
    }

    Release release;
    release.version = SemanticVersion::parse(root.value(QLatin1String("version")).toString());
    if (!release.version.valid()) {
        qCWarning(logNet) << "release feed names no valid version";
        return std::nullopt;
    }

    release.notes = root.value(QLatin1String("notes")).toString();
    release.publishedAt =
        QDateTime::fromString(root.value(QLatin1String("publishedAt")).toString(), Qt::ISODate);
    release.prerelease = root.value(QLatin1String("prerelease")).toBool();
    const QUrl changelog(root.value(QLatin1String("changelogUrl")).toString());
    if (secureUrl(changelog))
        release.changelogUrl = changelog;
    release.windowsInstaller = windowsInstallerFrom(root.value(QLatin1String("assets")).toArray());
    return release;
}

}
