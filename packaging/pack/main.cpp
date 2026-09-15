#include "Archive.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#include <filesystem>
#include <fstream>

namespace {

QTextStream &out()
{
    static QTextStream stream(stdout);
    return stream;
}

QTextStream &err()
{
    static QTextStream stream(stderr);
    return stream;
}

QString readable(quint64 bytes)
{
    if (bytes >= 1024ull * 1024 * 1024)
        return QStringLiteral("%1 GB").arg(double(bytes) / (1024 * 1024 * 1024), 0, 'f', 2);
    if (bytes >= 1024ull * 1024)
        return QStringLiteral("%1 MB").arg(double(bytes) / (1024 * 1024), 0, 'f', 1);
    return QStringLiteral("%1 KB").arg(double(bytes) / 1024, 0, 'f', 1);
}

bool appendBlob(std::ofstream &sink, const QString &root, int level, const QString &label,
                quint64 &offset, quint64 &size, setup::Digest &digest)
{
    std::string error;
    const std::vector<setup::ArchiveEntry> entries =
        setup::collect(std::filesystem::path(root.toStdU16String()), error);
    if (entries.empty()) {
        err() << "pack: "
              << (error.empty() ? QStringLiteral("%1 is empty").arg(root)
                                : QString::fromStdString(error))
              << Qt::endl;
        return false;
    }

    quint64 expanded = 0;
    for (const setup::ArchiveEntry &entry : entries)
        expanded += entry.size;

    offset = quint64(sink.tellp());
    QElapsedTimer clock;
    clock.start();

    setup::ArchiveWriter writer(level);
    if (!writer.write(sink, std::filesystem::path(root.toStdU16String()), entries, digest)) {
        err() << "pack: " << QString::fromStdString(writer.error()) << Qt::endl;
        return false;
    }

    size = quint64(sink.tellp()) - offset;
    out() << QStringLiteral("  %1  %2 files  %3 -> %4  (%5%)  %6 s")
                 .arg(label, -8)
                 .arg(qsizetype(entries.size()))
                 .arg(readable(expanded), readable(size))
                 .arg(expanded > 0 ? 100.0 * double(size) / double(expanded) : 0.0, 0, 'f', 1)
                 .arg(clock.elapsed() / 1000.0, 0, 'f', 1)
          << Qt::endl;
    return true;
}

bool copyInto(std::ofstream &sink, const QString &path)
{
    std::ifstream source(std::filesystem::path(path.toStdU16String()), std::ios::binary);
    if (!source) {
        err() << "pack: could not read " << path << Qt::endl;
        return false;
    }
    sink << source.rdbuf();
    if (!sink) {
        err() << "pack: could not copy " << path << Qt::endl;
        return false;
    }
    return true;
}

}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("htmusic-pack"));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Assembles the setup executable from a stub and two staged trees."));
    parser.addHelpOption();

    const QCommandLineOption stub({QStringLiteral("stub")},
                                  QStringLiteral("The native loader executable."),
                                  QStringLiteral("path"));
    const QCommandLineOption runtime({QStringLiteral("runtime")},
                                     QStringLiteral("The staged Qt runtime the installer needs."),
                                     QStringLiteral("dir"));
    const QCommandLineOption payload({QStringLiteral("payload")},
                                     QStringLiteral("The staged application."),
                                     QStringLiteral("dir"));
    const QCommandLineOption target({QStringLiteral("out")},
                                    QStringLiteral("The setup executable to write."),
                                    QStringLiteral("path"));
    const QCommandLineOption version({QStringLiteral("version")},
                                     QStringLiteral("The product version."),
                                     QStringLiteral("text"));
    const QCommandLineOption runtimeLevel({QStringLiteral("runtime-level")},
                                          QStringLiteral("Compression level for the runtime."),
                                          QStringLiteral("n"), QStringLiteral("12"));
    const QCommandLineOption payloadLevel({QStringLiteral("payload-level")},
                                          QStringLiteral("Compression level for the payload."),
                                          QStringLiteral("n"), QStringLiteral("19"));

    parser.addOptions({stub, runtime, payload, target, version, runtimeLevel, payloadLevel});
    parser.process(app);

    for (const QCommandLineOption &required : {stub, runtime, payload, target, version}) {
        if (!parser.isSet(required)) {
            err() << "pack: --" << required.names().constFirst() << " is required" << Qt::endl;
            return 2;
        }
    }

    const QString output = parser.value(target);
    QDir().mkpath(QFileInfo(output).absolutePath());

    std::ofstream sink(std::filesystem::path(output.toStdU16String()),
                       std::ios::binary | std::ios::trunc);
    if (!sink) {
        err() << "pack: could not write " << output << Qt::endl;
        return 1;
    }

    out() << "packing " << QFileInfo(output).fileName() << Qt::endl;

    if (!copyInto(sink, parser.value(stub)))
        return 1;

    setup::ArchiveFooter footer;
    footer.version = parser.value(version).toStdString();

    if (!appendBlob(sink, parser.value(runtime), parser.value(runtimeLevel).toInt(),
                    QStringLiteral("runtime"), footer.runtimeOffset, footer.runtimeSize,
                    footer.runtimeDigest)) {
        return 1;
    }
    if (!appendBlob(sink, parser.value(payload), parser.value(payloadLevel).toInt(),
                    QStringLiteral("payload"), footer.payloadOffset, footer.payloadSize,
                    footer.payloadDigest)) {
        return 1;
    }

    const std::string tail = footer.serialize();
    sink.write(tail.data(), std::streamsize(tail.size()));
    const quint64 total = quint64(sink.tellp());
    sink.close();
    if (!sink) {
        err() << "pack: could not finish " << output << Qt::endl;
        return 1;
    }

    out() << QStringLiteral("  total     %1").arg(readable(total)) << Qt::endl;
    return 0;
}
