/**
 * @file Registry.h
 * @brief Windows Registry operations wrapper for application settings
 *
 * Encapsulates all registry access under HKCU\Software\IDMClone.
 * Uses RAII for registry key handles to prevent leaks.
 * All operations are thread-safe with internal locking.
 */

#pragma once
#include "stdafx.h"

namespace idm {

class Registry {
public:
    static Registry& Instance();
    
    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;
    
    // ─── String values ─────────────────────────────────────────────────────
    bool WriteString(const String& subKey, const String& valueName, const String& data);
    String ReadString(const String& subKey, const String& valueName, const String& defaultValue = L"");
    
    // ─── Integer values ────────────────────────────────────────────────────
    bool WriteInt(const String& subKey, const String& valueName, DWORD data);
    DWORD ReadInt(const String& subKey, const String& valueName, DWORD defaultValue = 0);
    
    // ─── Boolean values (stored as DWORD 0/1) ──────────────────────────────
    bool WriteBool(const String& subKey, const String& valueName, bool data);
    bool ReadBool(const String& subKey, const String& valueName, bool defaultValue = false);
    
    // ─── Binary data ───────────────────────────────────────────────────────
    bool WriteBinary(const String& subKey, const String& valueName,
                     const std::vector<uint8>& data);
    std::vector<uint8> ReadBinary(const String& subKey, const String& valueName);
    
    // ─── Key operations ────────────────────────────────────────────────────
    bool DeleteValue(const String& subKey, const String& valueName);
    bool DeleteKey(const String& subKey);
    bool KeyExists(const String& subKey);
    
    // ─── Bulk settings operations ──────────────────────────────────────────
    // Convenience methods for common application settings
    
    struct AppSettings {
        bool    startWithWindows     = false;
        bool    startMinimized       = false;
        bool    clipboardMonitoring  = true;
        bool    darkMode             = false;
        bool    showInfoDialog       = true;
        bool    showCompleteDialog   = true;
        int     maxConnections       = 8;
        int     connectionTimeout    = 30;
        int     retryCount           = 20;
        int     retryDelay           = 5;
        String  defaultSaveDir;
        String  tempDir;
        String  fileTypes;
        String  userAgent;
        int     toolbarStyle         = 0;  // 0=Large3D, 1=Small3D, 2=LargeNeon, 3=SmallNeon
        int     progressShowMode     = 0;  // 0=Normal, 1=Minimized, 2=Don't show
        
        // Proxy settings
        int     proxyMode            = 0;  // 0=None, 1=System, 2=Manual
        String  httpProxyAddr;
        int     httpProxyPort        = 0;
        String  socksAddr;
        int     socksPort            = 0;
        int     socksType            = 5;  // 4 or 5
    };
    
    AppSettings LoadSettings();
    void SaveSettings(const AppSettings& settings);
    
private:
    Registry();
    ~Registry();
    
    // RAII wrapper for HKEY
    class RegKey {
    public:
        RegKey() : m_key(nullptr) {}
        ~RegKey() { Close(); }
        HKEY* operator&() { return &m_key; }
        operator HKEY() const { return m_key; }
        void Close() { if (m_key) { ::RegCloseKey(m_key); m_key = nullptr; } }
    private:
        HKEY m_key;
    };
    
    String FullKeyPath(const String& subKey) const;
    Mutex m_mutex;
};

} // namespace idm
