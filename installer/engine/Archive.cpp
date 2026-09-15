#include "Archive.h"

#include "Bytes.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <vector>

namespace {

constexpr std::size_t kReadChunk = 1 << 20;

}

namespace setup {

using namespace bytes;

std::string ArchiveFooter::serialize() const
{
    std::string raw(kSize, '\0');
    putNumber(raw, 0, kMagic, 8);
    putNumber(raw, 8, kFormat, 4);
    putBytes(raw, 12, version.data(), std::min<std::size_t>(version.size(), kVersionWidth));
    putNumber(raw, 28, runtimeOffset, 8);
    putNumber(raw, 36, runtimeSize, 8);
    putNumber(raw, 44, payloadOffset, 8);
    putNumber(raw, 52, payloadSize, 8);
    putBytes(raw, 60, runtimeDigest.data(), runtimeDigest.size());
    putBytes(raw, 92, payloadDigest.data(), payloadDigest.size());
    return raw;
}

bool ArchiveFooter::parse(const std::string &raw, ArchiveFooter &footer)
{
    if (raw.size() != kSize)
        return false;
    if (takeNumber(raw, 0, 8) != kMagic || takeNumber(raw, 8, 4) != kFormat)
        return false;

    footer.version.assign(raw, 12, kVersionWidth);
    const std::size_t end = footer.version.find('\0');
    if (end != std::string::npos)
        footer.version.resize(end);

    footer.runtimeOffset = takeNumber(raw, 28, 8);
    footer.runtimeSize = takeNumber(raw, 36, 8);
    footer.payloadOffset = takeNumber(raw, 44, 8);
    footer.payloadSize = takeNumber(raw, 52, 8);
    std::memcpy(footer.runtimeDigest.data(), raw.data() + 60, footer.runtimeDigest.size());
    std::memcpy(footer.payloadDigest.data(), raw.data() + 92, footer.payloadDigest.size());
    return true;
}

bool ArchiveFooter::readFrom(const std::filesystem::path &file, ArchiveFooter &footer)
{
    std::ifstream source(file, std::ios::binary);
    if (!source)
        return false;
    source.seekg(-static_cast<std::streamoff>(kSize), std::ios::end);
    if (!source)
        return false;

    std::string raw(kSize, '\0');
    if (!readExactly(source, raw.data(), kSize))
        return false;
    if (!parse(raw, footer))
        return false;
    const auto end = source.tellg();
    if (end < kSize)
        return false;
    const auto limit = std::uint64_t(end) - kSize;
    if (footer.runtimeOffset > limit || footer.runtimeSize > limit - footer.runtimeOffset)
        return false;
    const auto runtimeEnd = footer.runtimeOffset + footer.runtimeSize;
    return footer.payloadSize == 0 ? runtimeEnd == limit
                                   : footer.payloadOffset == runtimeEnd
            && footer.payloadOffset <= limit && footer.payloadSize == limit - footer.payloadOffset;
}

bool verifyRegion(std::istream &source, std::uint64_t offset, std::uint64_t size,
                  const Digest &digest)
{
    source.clear();
    source.seekg(std::streamoff(offset), std::ios::beg);
    if (!source)
        return false;

    Hasher hasher;
    if (!hasher.valid())
        return false;

    std::vector<char> buffer(kReadChunk);
    std::uint64_t left = size;
    while (left > 0) {
        const std::uint64_t want = std::min<std::uint64_t>(buffer.size(), left);
        source.read(buffer.data(), std::streamsize(want));
        const std::streamsize got = source.gcount();
        if (got <= 0)
            return false;
        hasher.add(buffer.data(), std::size_t(got));
        left -= std::uint64_t(got);
    }
    return hasher.result() == digest;
}

bool cloneWithoutPayload(const std::filesystem::path &origin, const std::filesystem::path &target)
{
    ArchiveFooter footer;
    if (!ArchiveFooter::readFrom(origin, footer))
        return false;

    std::ifstream source(origin, std::ios::binary);
    std::ofstream sink(target, std::ios::binary | std::ios::trunc);
    if (!source || !sink)
        return false;

    std::vector<char> buffer(kReadChunk);
    std::uint64_t left = footer.payloadOffset;
    while (left > 0) {
        const std::uint64_t want = std::min<std::uint64_t>(buffer.size(), left);
        source.read(buffer.data(), std::streamsize(want));
        const std::streamsize got = source.gcount();
        if (got <= 0)
            return false;
        sink.write(buffer.data(), got);
        left -= std::uint64_t(got);
    }

    footer.payloadOffset = 0;
    footer.payloadSize = 0;
    footer.payloadDigest = Digest {};
    const std::string tail = footer.serialize();
    sink.write(tail.data(), std::streamsize(tail.size()));
    sink.close();
    return bool(sink);
}

}
