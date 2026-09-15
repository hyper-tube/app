#pragma once

#include "SemanticVersion.h"

#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <QUrl>

#include <optional>

namespace update {

struct Release
{
    struct Package
    {
        QUrl url;
        QString fileName;
        QByteArray sha256;
        qint64 size = 0;

        bool verifiable() const { return url.isValid() && size > 0 && sha256.size() == 64; }
    };

    SemanticVersion version;
    QString notes;
    QDateTime publishedAt;
    QUrl changelogUrl;
    bool prerelease = false;
    Package windowsInstaller;

    static std::optional<Release> fromJson(const QByteArray &body);
};

}
