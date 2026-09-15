#include "Archive.h"

#include "Bytes.h"

#include <zstd.h>

#include <algorithm>
#include <fstream>
#include <ostream>
#include <vector>

namespace {

constexpr std::size_t kReadChunk = 1 << 20;

std::string relativeUtf8(const std::filesystem::path &root, const std::filesystem::path &file)
{
    const std::u8string text = std::filesystem::relative(file, root).generic_u8string();
    return std::string(text.begin(), text.end());
}

}

namespace setup {

using namespace bytes;

std::vector<ArchiveEntry> collect(const std::filesystem::path &root, std::string &error)
{
    std::vector<ArchiveEntry> entries;
    std::error_code code;
    if (!std::filesystem::is_directory(root, code)) {
        error = "the folder does not exist";
        return entries;
    }

    for (const auto &item : std::filesystem::recursive_directory_iterator(root, code)) {
        if (code) {
            error = code.message();
            return {};
        }
        if (!item.is_regular_file())
            continue;

        ArchiveEntry entry;
        entry.path = relativeUtf8(root, item.path());
        entry.size = std::uint64_t(item.file_size());
        if (!digestOf(item.path(), entry.digest)) {
            error = "could not read " + entry.path;
            return {};
        }
        entries.push_back(std::move(entry));
    }

    std::sort(entries.begin(), entries.end(),
              [](const ArchiveEntry &a, const ArchiveEntry &b) { return a.path < b.path; });
    return entries;
}

ArchiveWriter::ArchiveWriter(int level)
    : m_level(level)
{
}

bool ArchiveWriter::fail(const std::string &reason)
{
    m_error = reason;
    return false;
}

bool ArchiveWriter::write(std::ostream &sink, const std::filesystem::path &root,
                          const std::vector<ArchiveEntry> &entries, Digest &digest)
{
    std::string table;
    appendNumber(table, entries.size(), 4);
    for (const ArchiveEntry &entry : entries) {
        appendNumber(table, entry.path.size(), 2);
        table.append(entry.path);
        appendNumber(table, entry.size, 8);
        table.append(reinterpret_cast<const char *>(entry.digest.data()), entry.digest.size());
    }

    Hasher running;
    if (!running.valid())
        return fail("could not create a hasher");

    std::string prefix;
    appendNumber(prefix, table.size(), 4);
    sink.write(prefix.data(), std::streamsize(prefix.size()));
    sink.write(table.data(), std::streamsize(table.size()));
    running.add(prefix.data(), prefix.size());
    running.add(table.data(), table.size());

    ZSTD_CCtx *context = ZSTD_createCCtx();
    if (!context)
        return fail("could not create a compressor");
    ZSTD_CCtx_setParameter(context, ZSTD_c_compressionLevel, m_level);
    ZSTD_CCtx_setParameter(context, ZSTD_c_checksumFlag, 1);
    ZSTD_CCtx_setParameter(context, ZSTD_c_nbWorkers, 4);

    std::vector<char> sinkBuffer(ZSTD_CStreamOutSize());
    std::vector<char> sourceBuffer(kReadChunk);
    bool ok = true;

    const auto drain = [&](ZSTD_inBuffer &input, ZSTD_EndDirective mode) {
        std::size_t remaining = 0;
        do {
            ZSTD_outBuffer chunk {sinkBuffer.data(), sinkBuffer.size(), 0};
            remaining = ZSTD_compressStream2(context, &chunk, &input, mode);
            if (ZSTD_isError(remaining)) {
                ok = fail(ZSTD_getErrorName(remaining));
                return;
            }
            sink.write(sinkBuffer.data(), std::streamsize(chunk.pos));
            running.add(sinkBuffer.data(), chunk.pos);
        } while (mode == ZSTD_e_end ? remaining != 0 : input.pos != input.size);
    };

    for (const ArchiveEntry &entry : entries) {
        if (!ok)
            break;
        std::ifstream file(root / fromUtf8(entry.path), std::ios::binary);
        if (!file) {
            ok = fail("could not read " + entry.path);
            break;
        }
        while (ok && file) {
            file.read(sourceBuffer.data(), std::streamsize(sourceBuffer.size()));
            const std::streamsize got = file.gcount();
            if (got <= 0)
                break;
            ZSTD_inBuffer input {sourceBuffer.data(), std::size_t(got), 0};
            drain(input, ZSTD_e_continue);
        }
    }

    if (ok) {
        ZSTD_inBuffer input {sourceBuffer.data(), 0, 0};
        drain(input, ZSTD_e_end);
    }

    ZSTD_freeCCtx(context);
    digest = running.result();
    return ok;
}

}
