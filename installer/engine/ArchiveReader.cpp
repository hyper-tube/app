#include "Archive.h"

#include "Bytes.h"
#include "SafePath.h"

#include <zstd.h>

#include <algorithm>
#include <fstream>
#include <istream>
#include <limits>
#include <set>
#include <vector>

namespace setup {

using namespace bytes;

bool ArchiveReader::fail(const std::string &reason)
{
    m_error = reason;
    return false;
}

bool ArchiveReader::open(std::istream &source, std::uint64_t offset, std::uint64_t size)
{
    m_source = nullptr;
    m_error.clear();
    m_entries.clear();
    m_written.clear();
    m_expanded = 0;
    m_cancelled = false;

    source.clear();
    source.seekg(std::streamoff(offset), std::ios::beg);
    if (!source || size <= 4)
        return fail("the archive is truncated");

    std::string prefix(4, '\0');
    if (!readExactly(source, prefix.data(), 4))
        return fail("the archive is truncated");

    const std::uint64_t tableSize = takeNumber(prefix, 0, 4);
    if (tableSize < 4 || tableSize > 64 * 1024 * 1024 || tableSize + 4 >= size)
        return fail("the archive table is not readable");

    std::string table(std::size_t(tableSize), '\0');
    if (!readExactly(source, table.data(), tableSize))
        return fail("the archive table is truncated");

    std::set<std::string> names;
    std::size_t at = 0;
    const std::uint64_t count = takeNumber(table, at, 4);
    at += 4;
    for (std::uint64_t index = 0; index < count; ++index) {
        if (at + 2 > table.size())
            return fail("the archive table is malformed");
        const std::size_t length = std::size_t(takeNumber(table, at, 2));
        at += 2;
        if (at + length + 8 + 32 > table.size())
            return fail("the archive table is malformed");

        ArchiveEntry entry;
        entry.path.assign(table, at, length);
        at += length;
        entry.size = takeNumber(table, at, 8);
        at += 8;
        std::memcpy(entry.digest.data(), table.data() + at, entry.digest.size());
        at += entry.digest.size();

        if (!safeRelativePath(entry.path))
            return fail("the archive contains an unsafe path");
        std::string name = entry.path;
        std::transform(name.begin(), name.end(), name.begin(),
                       [](unsigned char c) { return char(std::tolower(c)); });
        if (!names.insert(name).second)
            return fail("the archive contains duplicate paths");
        if (entry.size > std::numeric_limits<std::uint64_t>::max() - m_expanded)
            return fail("the archive size is invalid");
        m_expanded += entry.size;
        m_entries.push_back(std::move(entry));
    }

    if (at != table.size())
        return fail("the archive table has trailing data");
    m_source = &source;
    m_streamOffset = offset + 4 + tableSize;
    m_streamSize = size - 4 - tableSize;
    return true;
}

bool ArchiveReader::extract(const std::filesystem::path &root, const Progress &progress)
{
    if (!m_source)
        return fail("the archive is not open");

    m_source->clear();
    m_source->seekg(std::streamoff(m_streamOffset), std::ios::beg);
    if (!*m_source)
        return fail("the archive is truncated");

    ZSTD_DCtx *context = ZSTD_createDCtx();
    if (!context)
        return fail("could not create a decompressor");

    std::vector<char> sourceBuffer(ZSTD_DStreamInSize());
    std::vector<char> sinkBuffer(ZSTD_DStreamOutSize());
    ZSTD_inBuffer input {sourceBuffer.data(), 0, 0};

    std::uint64_t available = m_streamSize;
    std::uint64_t done = 0;
    bool ok = true;
    bool ended = false;

    for (const ArchiveEntry &entry : m_entries) {
        if (!ok)
            break;

        if (progress && !progress(entry.path, done, m_expanded)) {
            m_cancelled = true;
            ok = fail("cancelled");
            break;
        }
        const std::filesystem::path target = root / fromUtf8(entry.path);
        if (!safeDestination(target)) {
            ok = fail("the destination contains a redirected folder");
            break;
        }
        std::error_code code;
        std::filesystem::create_directories(target.parent_path(), code);

        std::ofstream file(target, std::ios::binary | std::ios::trunc);
        if (!file) {
            ok = fail("could not write " + entry.path);
            break;
        }
        m_written.push_back(entry.path);

        Hasher hasher;
        std::uint64_t left = entry.size;
        while (left > 0) {
            if (input.pos == input.size) {
                const std::uint64_t want = std::min<std::uint64_t>(sourceBuffer.size(), available);
                if (want == 0) {
                    ok = fail("the archive ended early");
                    break;
                }
                m_source->read(sourceBuffer.data(), std::streamsize(want));
                const std::streamsize got = m_source->gcount();
                if (got <= 0) {
                    ok = fail("the archive ended early");
                    break;
                }
                available -= std::uint64_t(got);
                input.size = std::size_t(got);
                input.pos = 0;
            }

            ZSTD_outBuffer chunk {sinkBuffer.data(),
                                  std::size_t(std::min<std::uint64_t>(sinkBuffer.size(), left)), 0};
            const std::size_t status = ZSTD_decompressStream(context, &chunk, &input);
            if (ZSTD_isError(status)) {
                ok = fail(ZSTD_getErrorName(status));
                break;
            }
            ended = status == 0;
            if (ended && chunk.pos < left) {
                ok = fail("the archive ended early");
                break;
            }
            if (chunk.pos == 0)
                continue;

            file.write(sinkBuffer.data(), std::streamsize(chunk.pos));
            if (!file) {
                ok = fail("could not write " + entry.path);
                break;
            }
            hasher.add(sinkBuffer.data(), chunk.pos);
            left -= chunk.pos;
            done += chunk.pos;

            if (progress && !progress(entry.path, done, m_expanded)) {
                m_cancelled = true;
                ok = fail("cancelled");
                break;
            }
        }

        file.close();
        if (ok && !file)
            ok = fail("could not finish writing " + entry.path);
        if (ok && hasher.result() != entry.digest)
            ok = fail(entry.path + " did not survive the transfer");
    }

    if (ok && ended && (available != 0 || input.pos != input.size))
        ok = fail("the archive contains trailing data");
    while (ok && !ended) {
        if (input.pos == input.size && available > 0) {
            const auto want = std::min<std::uint64_t>(sourceBuffer.size(), available);
            if (!readExactly(*m_source, sourceBuffer.data(), want)) {
                ok = fail("the archive is truncated");
                break;
            }
            available -= want;
            input.size = std::size_t(want);
            input.pos = 0;
        }
        ZSTD_outBuffer chunk {sinkBuffer.data(), sinkBuffer.size(), 0};
        const auto before = input.pos;
        const auto status = ZSTD_decompressStream(context, &chunk, &input);
        if (ZSTD_isError(status) || chunk.pos > 0) {
            ok = fail("the archive stream does not match its table");
            break;
        }
        if (status == 0) {
            if (available != 0 || input.pos != input.size)
                ok = fail("the archive contains trailing data");
            break;
        }
        if (input.pos == before && available == 0) {
            ok = fail("the archive ended early");
            break;
        }
    }
    ZSTD_freeDCtx(context);
    return ok;
}

}
