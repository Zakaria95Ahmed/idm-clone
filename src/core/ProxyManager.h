/**
 * @file ProxyManager.h / ProxyManager.cpp
 * @brief Proxy/SOCKS configuration manager
 */

#pragma once
#include "stdafx.h"

namespace idm {

enum class ProxyType { None, System, Http, Socks4, Socks5 };

struct ProxyConfig {
    ProxyType   type{ProxyType::None};
    String      address;
    uint16      port{0};
    String      username;
    String      password;
    String      exceptions;  // Semicolon-separated bypass list
};

class ProxyManager {
public:
    static ProxyManager& Instance();
    
    ProxyConfig GetProxyForUrl(const String& url) const;
    void SetHttpProxy(const String& addr, uint16 port, const String& user = L"", const String& pass = L"");
    void SetSocksProxy(const String& addr, uint16 port, ProxyType socksVersion, const String& user = L"", const String& pass = L"");
    void SetSystemProxy();
    void SetNoProxy();
    void SetExceptions(const String& exceptions);
    
    ProxyConfig GetCurrentConfig() const { Lock lock(m_mutex); return m_config; }
    
    // Import proxy settings from Windows Internet Settings
    static ProxyConfig GetSystemProxy();
    
private:
    ProxyManager() = default;
    bool IsException(const String& host) const;
    
    ProxyConfig         m_config;
    mutable Mutex       m_mutex;
};

} // namespace idm
