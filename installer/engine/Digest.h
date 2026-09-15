#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace setup {

using Digest = std::array<std::uint8_t, 32>;

class Hasher
{
public:
    Hasher();
    ~Hasher();

    Hasher(const Hasher &) = delete;
    Hasher &operator=(const Hasher &) = delete;

    bool valid() const { return m_hash != nullptr; }

    void add(const void *data, std::size_t size);
    Digest result();

private:
    void *m_algorithm = nullptr;
    void *m_hash = nullptr;
    std::vector<unsigned char> m_object;
};

std::string hex(const Digest &digest);

bool digestOf(const std::filesystem::path &file, Digest &digest);

}
