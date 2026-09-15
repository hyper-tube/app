#include "Digest.h"

#include <windows.h>

#include <bcrypt.h>

#include <fstream>

namespace {

constexpr std::size_t kReadChunk = 1 << 20;

}

namespace setup {

Hasher::Hasher()
{
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    if (!BCRYPT_SUCCESS(
            BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0)))
        return;
    m_algorithm = algorithm;

    DWORD objectSize = 0;
    DWORD written = 0;
    if (!BCRYPT_SUCCESS(BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH,
                                          reinterpret_cast<PUCHAR>(&objectSize), sizeof(objectSize),
                                          &written, 0))) {
        return;
    }

    m_object.resize(objectSize);
    BCRYPT_HASH_HANDLE hash = nullptr;
    if (!BCRYPT_SUCCESS(
            BCryptCreateHash(algorithm, &hash, m_object.data(), objectSize, nullptr, 0, 0))) {
        return;
    }
    m_hash = hash;
}

Hasher::~Hasher()
{
    if (m_hash)
        BCryptDestroyHash(static_cast<BCRYPT_HASH_HANDLE>(m_hash));
    if (m_algorithm)
        BCryptCloseAlgorithmProvider(static_cast<BCRYPT_ALG_HANDLE>(m_algorithm), 0);
}

void Hasher::add(const void *data, std::size_t size)
{
    if (!m_hash || size == 0)
        return;
    BCryptHashData(static_cast<BCRYPT_HASH_HANDLE>(m_hash),
                   static_cast<PUCHAR>(const_cast<void *>(data)), ULONG(size), 0);
}

Digest Hasher::result()
{
    Digest digest {};
    if (m_hash)
        BCryptFinishHash(static_cast<BCRYPT_HASH_HANDLE>(m_hash), digest.data(),
                         ULONG(digest.size()), 0);
    return digest;
}

std::string hex(const Digest &digest)
{
    static constexpr char alphabet[] = "0123456789abcdef";
    std::string text;
    text.reserve(digest.size() * 2);
    for (std::uint8_t byte : digest) {
        text.push_back(alphabet[byte >> 4]);
        text.push_back(alphabet[byte & 0x0f]);
    }
    return text;
}

bool digestOf(const std::filesystem::path &file, Digest &digest)
{
    std::ifstream source(file, std::ios::binary);
    if (!source)
        return false;

    Hasher hasher;
    if (!hasher.valid())
        return false;

    std::vector<char> buffer(kReadChunk);
    while (source) {
        source.read(buffer.data(), std::streamsize(buffer.size()));
        const std::streamsize got = source.gcount();
        if (got > 0)
            hasher.add(buffer.data(), std::size_t(got));
    }
    if (source.bad())
        return false;

    digest = hasher.result();
    return true;
}

}
