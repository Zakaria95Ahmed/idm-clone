/**
 * @file Registry.cpp
 * @brief Windows Registry wrapper implementation
 */

#include "stdafx.h"
#include "Registry.h"
#include "Logger.h"

namespace idm {

Registry& Registry::Instance() {
    static Registry instance;
    return instance;
}

Registry::Registry() = default;
Registry::~Registry() = default;

String Registry::FullKeyPath(const String& subKey) const {
    if (subKey.empty()) return constants::REG_ROOT_KEY;
    return String(constants::REG_ROOT_KEY) + L"\\" + subKey;
}

// ─── String Operations ─────────────────────────────────────────────────────
bool Registry::WriteString(const String& subKey, const String& valueName, const String& data) {
    Lock lock(m_mutex);
    RegKey key;
    String fullPath = FullKeyPath(subKey);
    
    LONG result = ::RegCreateKeyExW(HKEY_CURRENT_USER, fullPath.c_str(),
        0, nullptr, 0, KEY_WRITE, nullptr, &key, nullptr);
    
    if (result != ERROR_SUCCESS) return false;
    
    result = ::RegSetValueExW(key, valueName.c_str(), 0, REG_SZ,
        reinterpret_cast<const BYTE*>(data.c_str()),
        static_cast<DWORD>((data.length() + 1) * sizeof(wchar_t)));
    
    return result == ERROR_SUCCESS;
}

String Registry::ReadString(const String& subKey, const String& valueName, const String& defaultValue) {
    Lock lock(m_mutex);
    RegKey key;
    String fullPath = FullKeyPath(subKey);
    
    LONG result = ::RegOpenKeyExW(HKEY_CURRENT_USER, fullPath.c_str(),
        0, KEY_READ, &key);
    
    if (result != ERROR_SUCCESS) return defaultValue;
    
    DWORD type = 0;
    DWORD size = 0;
    result = ::RegQueryValueExW(key, valueName.c_str(), nullptr, &type, nullptr, &size);
    
    if (result != ERROR_SUCCESS || type != REG_SZ || size == 0) return defaultValue;
    
    String data;
    data.resize(size / sizeof(wchar_t));
    result = ::RegQueryValueExW(key, valueName.c_str(), nullptr, nullptr,
        reinterpret_cast<BYTE*>(&data[0]), &size);
    
    if (result != ERROR_SUCCESS) return defaultValue;
    
    // Remove trailing null
    while (!data.empty() && data.back() == L'\0') data.pop_back();
    return data;
}

// ─── Integer Operations ────────────────────────────────────────────────────
bool Registry::WriteInt(const String& subKey, const String& valueName, DWORD data) {
    Lock lock(m_mutex);
    RegKey key;
    String fullPath = FullKeyPath(subKey);
    
    LONG result = ::RegCreateKeyExW(HKEY_CURRENT_USER, fullPath.c_str(),
        0, nullptr, 0, KEY_WRITE, nullptr, &key, nullptr);
    
    if (result != ERROR_SUCCESS) return false;
    
    result = ::RegSetValueExW(key, valueName.c_str(), 0, REG_DWORD,
        reinterpret_cast<const BYTE*>(&data), sizeof(DWORD));
    
    return result == ERROR_SUCCESS;
}

DWORD Registry::ReadInt(const String& subKey, const String& valueName, DWORD defaultValue) {
    Lock lock(m_mutex);
    RegKey key;
    String fullPath = FullKeyPath(subKey);
    
    LONG result = ::RegOpenKeyExW(HKEY_CURRENT_USER, fullPath.c_str(),
        0, KEY_READ, &key);
    
    if (result != ERROR_SUCCESS) return defaultValue;
    
    DWORD type = 0;
    DWORD data = 0;
    DWORD size = sizeof(DWORD);
    result = ::RegQueryValueExW(key, valueName.c_str(), nullptr, &type,
        reinterpret_cast<BYTE*>(&data), &size);
    
    return (result == ERROR_SUCCESS && type == REG_DWORD) ? data : defaultValue;
}

// ─── Boolean Operations ────────────────────────────────────────────────────
bool Registry::WriteBool(const String& subKey, const String& valueName, bool data) {
    return WriteInt(subKey, valueName, data ? 1 : 0);
}

bool Registry::ReadBool(const String& subKey, const String& valueName, bool defaultValue) {
    return ReadInt(subKey, valueName, defaultValue ? 1 : 0) != 0;
}

// ─── Binary Operations ────────────────────────────────────────────────────
bool Registry::WriteBinary(const String& subKey, const String& valueName,
                           const std::vector<uint8>& data) {
    Lock lock(m_mutex);
    RegKey key;
    String fullPath = FullKeyPath(subKey);
    
    LONG result = ::RegCreateKeyExW(HKEY_CURRENT_USER, fullPath.c_str(),
        0, nullptr, 0, KEY_WRITE, nullptr, &key, nullptr);
    
    if (result != ERROR_SUCCESS) return false;
    
    result = ::RegSetValueExW(key, valueName.c_str(), 0, REG_BINARY,
        data.data(), static_cast<DWORD>(data.size()));
    
    return result == ERROR_SUCCESS;
}

std::vector<uint8> Registry::ReadBinary(const String& subKey, const String& valueName) {
    Lock lock(m_mutex);
    RegKey key;
    String fullPath = FullKeyPath(subKey);
    
    LONG result = ::RegOpenKeyExW(HKEY_CURRENT_USER, fullPath.c_str(),
        0, KEY_READ, &key);
    
    if (result != ERROR_SUCCESS) return {};
    
    DWORD size = 0;
    result = ::RegQueryValueExW(key, valueName.c_str(), nullptr, nullptr, nullptr, &size);
    if (result != ERROR_SUCCESS || size == 0) return {};
    
    std::vector<uint8> data(size);
    result = ::RegQueryValueExW(key, valueName.c_str(), nullptr, nullptr, data.data(), &size);
    
    return (result == ERROR_SUCCESS) ? data : std::vector<uint8>{};
}

// ─── Key Operations ────────────────────────────────────────────────────────
bool Registry::DeleteValue(const String& subKey, const String& valueName) {
    Lock lock(m_mutex);
    RegKey key;
    String fullPath = FullKeyPath(subKey);
    
    LONG result = ::RegOpenKeyExW(HKEY_CURRENT_USER, fullPath.c_str(),
        0, KEY_SET_VALUE, &key);
    
    if (result != ERROR_SUCCESS) return false;
    return ::RegDeleteValueW(key, valueName.c_str()) == ERROR_SUCCESS;
}

bool Registry::DeleteKey(const String& subKey) {
    Lock lock(m_mutex);
    String fullPath = FullKeyPath(subKey);
    return ::RegDeleteTreeW(HKEY_CURRENT_USER, fullPath.c_str()) == ERROR_SUCCESS;
}

bool Registry::KeyExists(const String& subKey) {
    Lock lock(m_mutex);
    RegKey key;
    String fullPath = FullKeyPath(subKey);
    
    LONG result = ::RegOpenKeyExW(HKEY_CURRENT_USER, fullPath.c_str(),
        0, KEY_READ, &key);
    
    return result == ERROR_SUCCESS;
}

// ─── Settings Load/Save ────────────────────────────────────────────────────
Registry::AppSettings Registry::LoadSettings() {
    AppSettings s;
    
    String opts = L"Options";
    s.startWithWindows    = ReadBool(opts, L"StartWithWindows", false);
    s.startMinimized      = ReadBool(opts, L"StartMinimized", false);
    s.clipboardMonitoring = ReadBool(opts, L"ClipboardMonitoring", true);
    s.darkMode            = ReadBool(opts, L"DarkMode", false);
    s.showInfoDialog      = ReadBool(opts, L"ShowInfoDialog", true);
    s.showCompleteDialog  = ReadBool(opts, L"ShowCompleteDialog", true);
    s.maxConnections      = static_cast<int>(ReadInt(opts, L"MaxConnections", 8));
    s.connectionTimeout   = static_cast<int>(ReadInt(opts, L"Timeout", 30));
    s.retryCount          = static_cast<int>(ReadInt(opts, L"RetryCount", 20));
    s.retryDelay          = static_cast<int>(ReadInt(opts, L"RetryDelay", 5));
    s.toolbarStyle        = static_cast<int>(ReadInt(opts, L"ToolbarStyle", 0));
    s.progressShowMode    = static_cast<int>(ReadInt(opts, L"ProgressMode", 0));
    s.proxyMode           = static_cast<int>(ReadInt(opts, L"ProxyMode", 0));
    s.httpProxyPort       = static_cast<int>(ReadInt(opts, L"HttpProxyPort", 0));
    s.socksPort           = static_cast<int>(ReadInt(opts, L"SocksPort", 0));
    s.socksType           = static_cast<int>(ReadInt(opts, L"SocksType", 5));
    
    // Get default save directory (My Documents\Downloads if not set)
    wchar_t docsPath[MAX_PATH];
    ::SHGetFolderPathW(nullptr, CSIDL_PERSONAL, nullptr, 0, docsPath);
    String defaultDir = String(docsPath) + L"\\Downloads";
    
    s.defaultSaveDir = ReadString(opts, L"DefaultSaveDir", defaultDir);
    s.tempDir        = ReadString(opts, L"TempDir", s.defaultSaveDir);
    s.fileTypes      = ReadString(opts, L"FileTypes", constants::DEFAULT_FILE_TYPES);
    s.userAgent      = ReadString(opts, L"UserAgent", constants::DEFAULT_USER_AGENT);
    s.httpProxyAddr  = ReadString(opts, L"HttpProxyAddr", L"");
    s.socksAddr      = ReadString(opts, L"SocksAddr", L"");
    
    return s;
}

void Registry::SaveSettings(const AppSettings& s) {
    String opts = L"Options";
    
    WriteBool(opts, L"StartWithWindows", s.startWithWindows);
    WriteBool(opts, L"StartMinimized", s.startMinimized);
    WriteBool(opts, L"ClipboardMonitoring", s.clipboardMonitoring);
    WriteBool(opts, L"DarkMode", s.darkMode);
    WriteBool(opts, L"ShowInfoDialog", s.showInfoDialog);
    WriteBool(opts, L"ShowCompleteDialog", s.showCompleteDialog);
    WriteInt(opts, L"MaxConnections", s.maxConnections);
    WriteInt(opts, L"Timeout", s.connectionTimeout);
    WriteInt(opts, L"RetryCount", s.retryCount);
    WriteInt(opts, L"RetryDelay", s.retryDelay);
    WriteInt(opts, L"ToolbarStyle", s.toolbarStyle);
    WriteInt(opts, L"ProgressMode", s.progressShowMode);
    WriteInt(opts, L"ProxyMode", s.proxyMode);
    WriteInt(opts, L"HttpProxyPort", s.httpProxyPort);
    WriteInt(opts, L"SocksPort", s.socksPort);
    WriteInt(opts, L"SocksType", s.socksType);
    WriteString(opts, L"DefaultSaveDir", s.defaultSaveDir);
    WriteString(opts, L"TempDir", s.tempDir);
    WriteString(opts, L"FileTypes", s.fileTypes);
    WriteString(opts, L"UserAgent", s.userAgent);
    WriteString(opts, L"HttpProxyAddr", s.httpProxyAddr);
    WriteString(opts, L"SocksAddr", s.socksAddr);
}

} // namespace idm
