#pragma once

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <istream>
#include <string>

namespace setup::bytes {

inline void putNumber(std::string &raw, std::size_t at, std::uint64_t value, int width)
{
    for (int byte = 0; byte < width; ++byte)
        raw[at + std::size_t(byte)] = char((value >> (8 * byte)) & 0xff);
}

inline std::uint64_t takeNumber(const std::string &raw, std::size_t at, int width)
{
    std::uint64_t value = 0;
    for (int byte = 0; byte < width; ++byte)
        value |= std::uint64_t(static_cast<unsigned char>(raw[at + std::size_t(byte)]))
            << (8 * byte);
    return value;
}

inline void putBytes(std::string &raw, std::size_t at, const void *value, std::size_t size)
{
    std::memcpy(raw.data() + at, value, size);
}

inline void appendNumber(std::string &raw, std::uint64_t value, int width)
{
    const std::size_t at = raw.size();
    raw.resize(at + std::size_t(width));
    putNumber(raw, at, value, width);
}

inline bool readExactly(std::istream &source, char *into, std::uint64_t wanted)
{
    source.read(into, std::streamsize(wanted));
    return source.gcount() == std::streamsize(wanted);
}

inline std::filesystem::path fromUtf8(const std::string &text)
{
    return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t *>(text.c_str())));
}

}
