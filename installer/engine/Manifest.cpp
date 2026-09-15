#include "Manifest.h"

#include "SafePath.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>

namespace {

const QString kFileName = QStringLiteral("install.json");
const QString kVersion = QStringLiteral("version");
const QString kScope = QStringLiteral("scope");
const QString kComponents = QStringLiteral("components");
const QString kFiles = QStringLiteral("files");
const QString kAllUsers = QStringLiteral("machine");

}

namespace setup {

QString Manifest::pathIn(const QString &directory)
{
    return QDir(directory).filePath(kFileName);
}

Manifest Manifest::read(const QString &directory)
{
    Manifest manifest;
    QFile file(pathIn(directory));
    if (!file.open(QIODevice::ReadOnly))
        return manifest;

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        return manifest;
    const QJsonObject root = document.object();
    manifest.version = root.value(kVersion).toString();
    manifest.language = root.value(QStringLiteral("language")).toString();
    manifest.scope =
        root.value(kScope).toString() == kAllUsers ? Scope::AllUsers : Scope::CurrentUser;
    manifest.components = Components::fromInt(root.value(kComponents).toInt());
    const QJsonArray files = root.value(kFiles).toArray();
    manifest.files.reserve(files.size());
    QSet<QString> names;
    for (const QJsonValue &entry : files) {
        const QString relative = entry.isObject()
            ? entry.toObject().value(QStringLiteral("path")).toString()
            : entry.toString();
        if (!safeRelativePath(relative.toStdString()) || names.contains(relative.toCaseFolded()))
            return {};
        names.insert(relative.toCaseFolded());
        manifest.files.append(relative);
        if (entry.isObject()) {
            manifest.sizes.insert(
                relative, entry.toObject().value(QStringLiteral("size")).toVariant().toULongLong());
            manifest.hashes.insert(relative,
                                   entry.toObject().value(QStringLiteral("sha256")).toString());
        }
    }
    manifest.valid = !manifest.version.isEmpty() && !manifest.files.isEmpty()
        && (root.value(kScope).toString() == kAllUsers
            || root.value(kScope).toString() == QLatin1String("user"));
    return manifest;
}

bool Manifest::write(const QString &directory) const
{
    QJsonArray entries;
    for (const QString &file : files) {
        QJsonObject entry;
        entry.insert(QStringLiteral("path"), file);
        entry.insert(QStringLiteral("size"), qint64(sizes.value(file)));
        entry.insert(QStringLiteral("sha256"), hashes.value(file));
        entries.append(entry);
    }

    QJsonObject root;
    root.insert(kVersion, version);
    root.insert(QStringLiteral("language"), language);
    root.insert(kScope, scope == Scope::AllUsers ? kAllUsers : QStringLiteral("user"));
    root.insert(kComponents, int(components.toInt()));
    root.insert(kFiles, entries);

    QSaveFile file(pathIn(directory));
    if (!file.open(QIODevice::WriteOnly))
        return false;
    const QByteArray bytes = QJsonDocument(root).toJson(QJsonDocument::Indented);
    return file.write(bytes) == bytes.size() && file.commit();
}

}
