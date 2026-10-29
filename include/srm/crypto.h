#pragma once

#include <cstddef>
#include <string>

namespace srm {

class Crypto {
public:
    // Generate a cryptographically random salt of the given byte length, returned as a hex string.
    static std::string generateSaltHex(std::size_t lengthBytes = 16);

    // Hash password using PBKDF2-HMAC-SHA256.
    // Stored format: pbkdf2_sha256$<iterations>$<salt_hex>$<hash_hex>
    static std::string hashPassword(const std::string& password,
                                    const std::string& saltHex = "",
                                    unsigned int iterations = 100000);

    // Verify candidate password against stored hash format.
    // Constant-time comparison is used to prevent timing side-channels.
    static bool verifyPassword(const std::string& password,
                               const std::string& storedHash);

    // Constant-time string equality check.
    static bool constantTimeEquals(const std::string& a, const std::string& b);
};

}  // namespace srm
