/**
 * @file Crypto.h
 * @brief Cryptographic hash computation for file integrity verification
 *
 * Uses Windows BCrypt API (modern CNG - Cryptography Next Generation)
 * for computing MD5, SHA-1, and SHA-256 hashes. BCrypt is preferred over
 * the legacy CryptoAPI because:
 *   1. It's the recommended API for Windows Vista+ (all our targets)
 *   2. It supports hardware acceleration where available
 *   3. It's FIPS 140-2 compliant
 *   4. No OpenSSL dependency needed
 *
 * Usage:
 *   String hash = Crypto::FileHash(filePath, HashAlgorithm::SHA256);
 *   bool ok = Crypto::VerifyHash(filePath, expectedHash, HashAlgorithm::SHA256);
 */

#pragma once
#include "stdafx.h"

namespace idm {

enum class HashAlgorithm {
    MD5,
    SHA1,
    SHA256,
    CRC32
};

class Crypto {
public:
    /**
     * Compute hash of a file.
     * @param filePath  Path to the file
     * @param algorithm Hash algorithm to use
     * @return Hex-encoded hash string, or empty string on error
     */
    static String FileHash(const String& filePath, HashAlgorithm algorithm);
    
    /**
     * Compute hash of in-memory data.
     */
    static String DataHash(const uint8* data, size_t length, HashAlgorithm algorithm);
    
    /**
     * Verify a file against an expected hash.
     */
    static bool VerifyHash(const String& filePath, const String& expectedHash,
                           HashAlgorithm algorithm);
    
    /**
     * Compute CRC32 of a file (separate from BCrypt path).
     */
    static uint32 FileCRC32(const String& filePath);
    
    /**
     * Compute CRC32 of in-memory data.
     */
    static uint32 DataCRC32(const uint8* data, size_t length);
    
    /**
     * Convert binary hash to hex string.
     */
    static String ToHexString(const uint8* data, size_t length);
    
    /**
     * Parse a hash algorithm name string.
     */
    static HashAlgorithm ParseAlgorithm(const String& name);
};

} // namespace idm
