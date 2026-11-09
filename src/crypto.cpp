#include "srm/crypto.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <fstream>
#include <limits>
#include <stdexcept>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>
#endif
#include <sstream>
#include <vector>

namespace srm {
namespace {

#ifndef _WIN32

// Standard SHA-256 implementation conforming to FIPS 180-4 / RFC 6234
class Sha256 {
public:
    Sha256() { reset(); }

    void reset() {
        state_[0] = 0x6a09e667;
        state_[1] = 0xbb67ae85;
        state_[2] = 0x3c6ef372;
        state_[3] = 0xa54ff53a;
        state_[4] = 0x510e527f;
        state_[5] = 0x9b05688c;
        state_[6] = 0x1f83d9ab;
        state_[7] = 0x5be0cd19;
        count_ = 0;
        buffer_.fill(0);
    }

    void update(const uint8_t* data, size_t length) {
        for (size_t i = 0; i < length; ++i) {
            buffer_[count_ % 64] = data[i];
            count_++;
            if (count_ % 64 == 0) {
                transform(buffer_.data());
            }
        }
    }

    void finalize(std::array<uint8_t, 32>& digest) {
        uint64_t totalBits = count_ * 8;
        update(reinterpret_cast<const uint8_t*>("\x80"), 1);

        while (count_ % 64 != 56) {
            update(reinterpret_cast<const uint8_t*>("\x00"), 1);
        }

        std::array<uint8_t, 8> lengthBytes{};
        for (int i = 0; i < 8; ++i) {
            lengthBytes[i] = static_cast<uint8_t>((totalBits >> ((7 - i) * 8)) & 0xFF);
        }
        update(lengthBytes.data(), 8);

        for (int i = 0; i < 8; ++i) {
            digest[i * 4 + 0] = static_cast<uint8_t>((state_[i] >> 24) & 0xFF);
            digest[i * 4 + 1] = static_cast<uint8_t>((state_[i] >> 16) & 0xFF);
            digest[i * 4 + 2] = static_cast<uint8_t>((state_[i] >> 8) & 0xFF);
            digest[i * 4 + 3] = static_cast<uint8_t>(state_[i] & 0xFF);
        }
    }

private:
    static inline uint32_t rotr(uint32_t x, uint32_t n) {
        return (x >> n) | (x << (32 - n));
    }
    static inline uint32_t ch(uint32_t x, uint32_t y, uint32_t z) {
        return (x & y) ^ (~x & z);
    }
    static inline uint32_t maj(uint32_t x, uint32_t y, uint32_t z) {
        return (x & y) ^ (x & z) ^ (y & z);
    }
    static inline uint32_t sig0(uint32_t x) {
        return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
    }
    static inline uint32_t sig1(uint32_t x) {
        return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
    }
    static inline uint32_t gam0(uint32_t x) {
        return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
    }
    static inline uint32_t gam1(uint32_t x) {
        return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
    }

    void transform(const uint8_t* block) {
        static const uint32_t K[64] = {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
            0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
            0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
            0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
            0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
            0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
            0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
            0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
            0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
        };

        std::array<uint32_t, 64> w{};
        for (int i = 0; i < 16; ++i) {
            w[i] = (static_cast<uint32_t>(block[i * 4]) << 24) |
                   (static_cast<uint32_t>(block[i * 4 + 1]) << 16) |
                   (static_cast<uint32_t>(block[i * 4 + 2]) << 8) |
                   (static_cast<uint32_t>(block[i * 4 + 3]));
        }
        for (int i = 16; i < 64; ++i) {
            w[i] = gam1(w[i - 2]) + w[i - 7] + gam0(w[i - 15]) + w[i - 16];
        }

        uint32_t a = state_[0];
        uint32_t b = state_[1];
        uint32_t c = state_[2];
        uint32_t d = state_[3];
        uint32_t e = state_[4];
        uint32_t f = state_[5];
        uint32_t g = state_[6];
        uint32_t h = state_[7];

        for (int i = 0; i < 64; ++i) {
            uint32_t t1 = h + sig1(e) + ch(e, f, g) + K[i] + w[i];
            uint32_t t2 = sig0(a) + maj(a, b, c);
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }

        state_[0] += a;
        state_[1] += b;
        state_[2] += c;
        state_[3] += d;
        state_[4] += e;
        state_[5] += f;
        state_[6] += g;
        state_[7] += h;
    }

    std::array<uint32_t, 8> state_{};
    std::array<uint8_t, 64> buffer_{};
    uint64_t count_ = 0;
};

// Standard HMAC-SHA256 conforming to RFC 2104
std::array<uint8_t, 32> hmacSha256(const uint8_t* key, size_t keyLen,
                                   const uint8_t* data, size_t dataLen) {
    std::array<uint8_t, 64> keyBlock{};
    if (keyLen > 64) {
        Sha256 hKey;
        hKey.update(key, keyLen);
        std::array<uint8_t, 32> d{};
        hKey.finalize(d);
        std::memcpy(keyBlock.data(), d.data(), 32);
    } else {
        std::memcpy(keyBlock.data(), key, keyLen);
    }

    std::array<uint8_t, 64> iPad = keyBlock;
    std::array<uint8_t, 64> oPad = keyBlock;
    for (int i = 0; i < 64; ++i) {
        iPad[i] ^= 0x36;
        oPad[i] ^= 0x5C;
    }

    Sha256 inner;
    inner.update(iPad.data(), 64);
    inner.update(data, dataLen);
    std::array<uint8_t, 32> innerHash{};
    inner.finalize(innerHash);

    Sha256 outer;
    outer.update(oPad.data(), 64);
    outer.update(innerHash.data(), 32);
    std::array<uint8_t, 32> output{};
    outer.finalize(output);

    return output;
}

#endif

// Standard PBKDF2 with HMAC-SHA256 conforming to RFC 2898 / RFC 8018

std::array<uint8_t, 32> pbkdf2HmacSha256(
    const std::string& password,
    const std::vector<uint8_t>& salt,
    unsigned int iterations) {

    if (iterations == 0) {
        throw std::invalid_argument("PBKDF2 iterations must be greater than zero.");
    }

    std::array<uint8_t, 32> result{};

#ifdef _WIN32
    // Use Windows CNG for native PBKDF2-HMAC-SHA256.
    if (password.size() > std::numeric_limits<ULONG>::max() ||
        salt.size() > std::numeric_limits<ULONG>::max()) {
        throw std::length_error("Password or salt is too large.");
    }

    BCRYPT_ALG_HANDLE algorithm = nullptr;

    NTSTATUS status = BCryptOpenAlgorithmProvider(
        &algorithm,
        BCRYPT_SHA256_ALGORITHM,
        nullptr,
        BCRYPT_ALG_HANDLE_HMAC_FLAG
    );

    if (status < 0) {
        throw std::runtime_error(
            "Failed to initialize Windows SHA-256 provider."
        );
    }

    status = BCryptDeriveKeyPBKDF2(
        algorithm,
        reinterpret_cast<PUCHAR>(
            const_cast<char*>(password.data())),
        static_cast<ULONG>(password.size()),
        reinterpret_cast<PUCHAR>(
            const_cast<uint8_t*>(salt.data())),
        static_cast<ULONG>(salt.size()),
        static_cast<ULONGLONG>(iterations),
        result.data(),
        static_cast<ULONG>(result.size()),
        0
    );

    BCryptCloseAlgorithmProvider(algorithm, 0);

    if (status < 0) {
        throw std::runtime_error(
            "Windows PBKDF2-HMAC-SHA256 derivation failed."
        );
    }

#else
    // Portable C++ PBKDF2-HMAC-SHA256 implementation.
    std::vector<uint8_t> saltAndIndex = salt;
    saltAndIndex.push_back(0);
    saltAndIndex.push_back(0);
    saltAndIndex.push_back(0);
    saltAndIndex.push_back(1);

    const auto* pwdBytes =
        reinterpret_cast<const uint8_t*>(password.data());

    auto u = hmacSha256(
        pwdBytes, password.size(),
        saltAndIndex.data(), saltAndIndex.size()
    );

    result = u;

    for (unsigned int iter = 1; iter < iterations; ++iter) {
        u = hmacSha256(
            pwdBytes, password.size(),
            u.data(), u.size()
        );

        for (size_t b = 0; b < result.size(); ++b) {
            result[b] ^= u[b];
        }
    }
#endif

    return result;
}

std::string bytesToHex(const uint8_t* data, size_t length) {
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < length; ++i) {
        ss << std::setw(2) << static_cast<int>(data[i]);
    }
    return ss.str();
}

std::vector<uint8_t> hexToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    if (hex.size() % 2 != 0) {
        return bytes;
    }
    for (size_t i = 0; i < hex.size(); i += 2) {
        auto byte = static_cast<uint8_t>(std::stoi(hex.substr(i, 2), nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

}  // namespace

std::string Crypto::generateSaltHex(std::size_t lengthBytes) {
    if (lengthBytes == 0) {
        throw std::invalid_argument("Salt length must be greater than zero.");
    }

    std::vector<uint8_t> salt(lengthBytes);

#ifdef _WIN32
    if (lengthBytes > std::numeric_limits<ULONG>::max()) {
        throw std::length_error("Salt length is too large.");
    }

    // Use the Windows system-preferred cryptographic random generator.
    const NTSTATUS status = BCryptGenRandom(
        nullptr,
        salt.data(),
        static_cast<ULONG>(salt.size()),
        BCRYPT_USE_SYSTEM_PREFERRED_RNG
    );

    if (status < 0) {
        throw std::runtime_error("Failed to generate cryptographically secure salt.");
    }
#else
    // Use the operating system's random source on Unix-like platforms.
    std::ifstream randomSource("/dev/urandom", std::ios::binary);

    if (!randomSource ||
        !randomSource.read(
            reinterpret_cast<char*>(salt.data()),
            static_cast<std::streamsize>(salt.size()))) {
        throw std::runtime_error("Failed to generate cryptographically secure salt.");
    }
#endif

    return bytesToHex(salt.data(), salt.size());
}

std::string Crypto::hashPassword(const std::string& password,
                                 const std::string& saltHex,
                                 unsigned int iterations) {
    std::string actualSaltHex = saltHex.empty() ? generateSaltHex(16) : saltHex;
    std::vector<uint8_t> saltBytes = hexToBytes(actualSaltHex);
    auto derivedKey = pbkdf2HmacSha256(password, saltBytes, iterations);
    std::string hashHex = bytesToHex(derivedKey.data(), derivedKey.size());

    // Format: pbkdf2_sha256$<iterations>$<salt_hex>$<hash_hex>
    std::ostringstream formatted;
    formatted << "pbkdf2_sha256$" << iterations << "$" << actualSaltHex << "$" << hashHex;
    return formatted.str();
}

bool Crypto::constantTimeEquals(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) {
        return false;
    }
    unsigned char result = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        result |= static_cast<unsigned char>(a[i] ^ b[i]);
    }
    return result == 0;
}

bool Crypto::verifyPassword(const std::string& password,
                            const std::string& storedHash) {
    // Parse format: algorithm$iterations$salt$hash
    std::istringstream stream(storedHash);
    std::string alg;
    std::string iterStr;
    std::string saltHex;
    std::string expectedHashHex;

    if (!std::getline(stream, alg, '$') ||
        !std::getline(stream, iterStr, '$') ||
        !std::getline(stream, saltHex, '$') ||
        !std::getline(stream, expectedHashHex)) {
        return false;
    }

    if (alg != "pbkdf2_sha256" || saltHex.empty() || expectedHashHex.empty()) {
        return false;
    }

    unsigned int iterations = 100000;
    try {
        iterations = static_cast<unsigned int>(std::stoul(iterStr));
    } catch (...) {
        return false;
    }

    std::vector<uint8_t> saltBytes = hexToBytes(saltHex);
    auto derivedKey = pbkdf2HmacSha256(password, saltBytes, iterations);
    std::string computedHashHex = bytesToHex(derivedKey.data(), derivedKey.size());

    return constantTimeEquals(computedHashHex, expectedHashHex);
}

}  // namespace srm
