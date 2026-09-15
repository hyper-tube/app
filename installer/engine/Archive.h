#pragma once

#include "Digest.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <iosfwd>
#include <string>
#include <vector>

namespace setup {

struct ArchiveEntry
{
    std::string path;
    std::uint64_t size = 0;
    Digest digest {};
};

struct ArchiveFooter
{
    static constexpr std::uint64_t kMagic = 0x5054455355544d48ull;
    static constexpr std::uint32_t kFormat = 1;
    static constexpr int kSize = 128;
    static constexpr int kVersionWidth = 16;

    std::string version;
    std::uint64_t runtimeOffset = 0;
    std::uint64_t runtimeSize = 0;
    Digest runtimeDigest {};
    std::uint64_t payloadOffset = 0;
    std::uint64_t payloadSize = 0;
    Digest payloadDigest {};

    std::string serialize() const;
    static bool parse(const std::string &raw, ArchiveFooter &footer);
    static bool readFrom(const std::filesystem::path &file, ArchiveFooter &footer);
};

std::vector<ArchiveEntry> collect(const std::filesystem::path &root, std::string &error);

class ArchiveWriter
{
public:
    explicit ArchiveWriter(int level);

    bool write(std::ostream &sink, const std::filesystem::path &root,
               const std::vector<ArchiveEntry> &entries, Digest &digest);

    const std::string &error() const { return m_error; }

private:
    bool fail(const std::string &reason);

    int m_level;
    std::string m_error;
};

class ArchiveReader
{
public:
    using Progress =
        std::function<bool(const std::string &path, std::uint64_t done, std::uint64_t total)>;

    bool open(std::istream &source, std::uint64_t offset, std::uint64_t size);
    bool extract(const std::filesystem::path &root, const Progress &progress);

    const std::vector<ArchiveEntry> &entries() const { return m_entries; }
    std::uint64_t expandedSize() const { return m_expanded; }
    const std::vector<std::string> &written() const { return m_written; }
    bool cancelled() const { return m_cancelled; }
    const std::string &error() const { return m_error; }

private:
    bool fail(const std::string &reason);

    std::istream *m_source = nullptr;
    std::uint64_t m_streamOffset = 0;
    std::uint64_t m_streamSize = 0;
    std::uint64_t m_expanded = 0;
    bool m_cancelled = false;
    std::vector<ArchiveEntry> m_entries;
    std::vector<std::string> m_written;
    std::string m_error;
};

bool verifyRegion(std::istream &source, std::uint64_t offset, std::uint64_t size,
                  const Digest &digest);

bool cloneWithoutPayload(const std::filesystem::path &origin, const std::filesystem::path &target);

}
