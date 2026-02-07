/**
 * @file Crypto.cpp
 * @brief Hash computation implementation using Windows BCrypt
 */

#include "stdafx.h"
#include "Crypto.h"
#include "Logger.h"

namespace idm {

// ─── BCrypt Hash Wrapper ───────────────────────────────────────────────────
// RAII wrapper for BCrypt algorithm and hash handles
class BCryptHasher {
public:
    BCryptHasher(LPCWSTR algId) {
        NTSTATUS status = ::BCryptOpenAlgorithmProvider(&m_algorithm, algId, nullptr, 0);
        if (!BCRYPT_SUCCESS(status)) return;
        
        DWORD hashObjSize = 0, dataSize = 0;
        status = ::BCryptGetProperty(m_algorithm, BCRYPT_OBJECT_LENGTH,
            reinterpret_cast<PUCHAR>(&hashObjSize), sizeof(DWORD), &dataSize, 0);
        if (!BCRYPT_SUCCESS(status)) return;
        
        status = ::BCryptGetProperty(m_algorithm, BCRYPT_HASH_LENGTH,
            reinterpret_cast<PUCHAR>(&m_hashLength), sizeof(DWORD), &dataSize, 0);
        if (!BCRYPT_SUCCESS(status)) return;
        
        m_hashObject.resize(hashObjSize);
        status = ::BCryptCreateHash(m_algorithm, &m_hash,
            m_hashObject.data(), hashObjSize, nullptr, 0, 0);
        m_valid = BCRYPT_SUCCESS(status);
    }
    
    ~BCryptHasher() {
        if (m_hash) ::BCryptDestroyHash(m_hash);
        if (m_algorithm) ::BCryptCloseAlgorithmProvider(m_algorithm, 0);
    }
    
    bool Update(const uint8* data, size_t length) {
        if (!m_valid) return false;
        NTSTATUS status = ::BCryptHashData(m_hash,
            const_cast<PUCHAR>(data), static_cast<ULONG>(length), 0);
        return BCRYPT_SUCCESS(status);
    }
    
    std::vector<uint8> Finalize() {
        if (!m_valid) return {};
        std::vector<uint8> hash(m_hashLength);
        NTSTATUS status = ::BCryptFinishHash(m_hash, hash.data(), m_hashLength, 0);
        return BCRYPT_SUCCESS(status) ? hash : std::vector<uint8>{};
    }
    
    bool IsValid() const { return m_valid; }
    
private:
    BCRYPT_ALG_HANDLE   m_algorithm = nullptr;
    BCRYPT_HASH_HANDLE  m_hash = nullptr;
    std::vector<uint8>  m_hashObject;
    DWORD               m_hashLength = 0;
    bool                m_valid = false;
};

// ─── Algorithm ID Mapping ──────────────────────────────────────────────────
static LPCWSTR GetBCryptAlgId(HashAlgorithm alg) {
    switch (alg) {
        case HashAlgorithm::MD5:    return BCRYPT_MD5_ALGORITHM;
        case HashAlgorithm::SHA1:   return BCRYPT_SHA1_ALGORITHM;
        case HashAlgorithm::SHA256: return BCRYPT_SHA256_ALGORITHM;
        default: return nullptr;
    }
}

// ─── File Hash ─────────────────────────────────────────────────────────────
String Crypto::FileHash(const String& filePath, HashAlgorithm algorithm) {
    if (algorithm == HashAlgorithm::CRC32) {
        uint32 crc = FileCRC32(filePath);
        wchar_t buf[16];
        _snwprintf_s(buf, _TRUNCATE, L"%08X", crc);
        return buf;
    }
    
    LPCWSTR algId = GetBCryptAlgId(algorithm);
    if (!algId) return L"";
    
    BCryptHasher hasher(algId);
    if (!hasher.IsValid()) {
        LOG_ERROR(L"Failed to initialize hash algorithm");
        return L"";
    }
    
    // Read file in chunks and feed to hasher
    HANDLE hFile = ::CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ,
        nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    
    if (hFile == INVALID_HANDLE_VALUE) {
        LOG_ERROR(L"Cannot open file for hashing: %s", filePath.c_str());
        return L"";
    }
    
    std::vector<uint8> buffer(65536);
    DWORD bytesRead = 0;
    
    while (::ReadFile(hFile, buffer.data(), static_cast<DWORD>(buffer.size()),
                      &bytesRead, nullptr) && bytesRead > 0) {
        if (!hasher.Update(buffer.data(), bytesRead)) {
            ::CloseHandle(hFile);
            return L"";
        }
    }
    
    ::CloseHandle(hFile);
    
    auto hash = hasher.Finalize();
    return ToHexString(hash.data(), hash.size());
}

// ─── Data Hash ─────────────────────────────────────────────────────────────
String Crypto::DataHash(const uint8* data, size_t length, HashAlgorithm algorithm) {
    if (algorithm == HashAlgorithm::CRC32) {
        uint32 crc = DataCRC32(data, length);
        wchar_t buf[16];
        _snwprintf_s(buf, _TRUNCATE, L"%08X", crc);
        return buf;
    }
    
    LPCWSTR algId = GetBCryptAlgId(algorithm);
    if (!algId) return L"";
    
    BCryptHasher hasher(algId);
    if (!hasher.IsValid() || !hasher.Update(data, length)) return L"";
    
    auto hash = hasher.Finalize();
    return ToHexString(hash.data(), hash.size());
}

// ─── Verify Hash ───────────────────────────────────────────────────────────
bool Crypto::VerifyHash(const String& filePath, const String& expectedHash,
                        HashAlgorithm algorithm) {
    String computed = FileHash(filePath, algorithm);
    if (computed.empty()) return false;
    
    // Case-insensitive comparison
    return _wcsicmp(computed.c_str(), expectedHash.c_str()) == 0;
}

// ─── CRC32 ─────────────────────────────────────────────────────────────────
// Standard CRC32 implementation with lookup table (polynomial 0xEDB88320)
static uint32 s_crc32Table[256];
static bool s_crc32TableInit = false;

static void InitCRC32Table() {
    if (s_crc32TableInit) return;
    for (uint32 i = 0; i < 256; ++i) {
        uint32 crc = i;
        for (int j = 0; j < 8; ++j) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
        s_crc32Table[i] = crc;
    }
    s_crc32TableInit = true;
}

uint32 Crypto::DataCRC32(const uint8* data, size_t length) {
    InitCRC32Table();
    uint32 crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; ++i) {
        crc = s_crc32Table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}

uint32 Crypto::FileCRC32(const String& filePath) {
    InitCRC32Table();
    HANDLE hFile = ::CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ,
        nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    
    if (hFile == INVALID_HANDLE_VALUE) return 0;
    
    uint32 crc = 0xFFFFFFFF;
    std::vector<uint8> buffer(65536);
    DWORD bytesRead = 0;
    
    while (::ReadFile(hFile, buffer.data(), static_cast<DWORD>(buffer.size()),
                      &bytesRead, nullptr) && bytesRead > 0) {
        for (DWORD i = 0; i < bytesRead; ++i) {
            crc = s_crc32Table[(crc ^ buffer[i]) & 0xFF] ^ (crc >> 8);
        }
    }
    
    ::CloseHandle(hFile);
    return crc ^ 0xFFFFFFFF;
}

// ─── Hex String Conversion ─────────────────────────────────────────────────
String Crypto::ToHexString(const uint8* data, size_t length) {
    static const wchar_t hexChars[] = L"0123456789abcdef";
    String result;
    result.reserve(length * 2);
    for (size_t i = 0; i < length; ++i) {
        result += hexChars[(data[i] >> 4) & 0x0F];
        result += hexChars[data[i] & 0x0F];
    }
    return result;
}

// ─── Parse Algorithm Name ──────────────────────────────────────────────────
HashAlgorithm Crypto::ParseAlgorithm(const String& name) {
    if (_wcsicmp(name.c_str(), L"MD5") == 0) return HashAlgorithm::MD5;
    if (_wcsicmp(name.c_str(), L"SHA1") == 0 || _wcsicmp(name.c_str(), L"SHA-1") == 0) 
        return HashAlgorithm::SHA1;
    if (_wcsicmp(name.c_str(), L"SHA256") == 0 || _wcsicmp(name.c_str(), L"SHA-256") == 0) 
        return HashAlgorithm::SHA256;
    if (_wcsicmp(name.c_str(), L"CRC32") == 0) return HashAlgorithm::CRC32;
    return HashAlgorithm::SHA256; // Default
}

} // namespace idm
