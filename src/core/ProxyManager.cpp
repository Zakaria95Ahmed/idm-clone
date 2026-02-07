/**
 * @file ProxyManager.cpp
 * @brief Proxy configuration implementation
 */

#include "stdafx.h"
#include "ProxyManager.h"
#include "../util/Logger.h"
#include "../util/Unicode.h"

namespace idm {

ProxyManager& ProxyManager::Instance() {
    static ProxyManager instance;
    return instance;
}

ProxyConfig ProxyManager::GetProxyForUrl(const String& url) const {
    Lock lock(m_mutex);
    
    if (m_config.type == ProxyType::None) return {};
    
    // Check exceptions
    String host = Unicode::ExtractHostFromUrl(url);
    if (IsException(host)) return {};
    
    if (m_config.type == ProxyType::System) {
        return GetSystemProxy();
    }
    
    return m_config;
}

void ProxyManager::SetHttpProxy(const String& addr, uint16 port, 
                                 const String& user, const String& pass) {
    Lock lock(m_mutex);
    m_config.type = ProxyType::Http;
    m_config.address = addr;
    m_config.port = port;
    m_config.username = user;
    m_config.password = pass;
    LOG_INFO(L"Proxy: set HTTP proxy %s:%d", addr.c_str(), port);
}

void ProxyManager::SetSocksProxy(const String& addr, uint16 port, ProxyType socksVersion,
                                  const String& user, const String& pass) {
    Lock lock(m_mutex);
    m_config.type = socksVersion;
    m_config.address = addr;
    m_config.port = port;
    m_config.username = user;
    m_config.password = pass;
}

void ProxyManager::SetSystemProxy() {
    Lock lock(m_mutex);
    m_config.type = ProxyType::System;
}

void ProxyManager::SetNoProxy() {
    Lock lock(m_mutex);
    m_config = {};
}

void ProxyManager::SetExceptions(const String& exceptions) {
    Lock lock(m_mutex);
    m_config.exceptions = exceptions;
}

ProxyConfig ProxyManager::GetSystemProxy() {
    ProxyConfig result;
    
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG ieConfig = {};
    if (::WinHttpGetIEProxyConfigForCurrentUser(&ieConfig)) {
        if (ieConfig.lpszProxy) {
            result.type = ProxyType::Http;
            String proxy = ieConfig.lpszProxy;
            auto colonPos = proxy.rfind(L':');
            if (colonPos != String::npos) {
                result.address = proxy.substr(0, colonPos);
                try { result.port = static_cast<uint16>(std::stoi(proxy.substr(colonPos + 1))); }
                catch (...) { result.port = 8080; }
            } else {
                result.address = proxy;
                result.port = 8080;
            }
            ::GlobalFree(ieConfig.lpszProxy);
        }
        if (ieConfig.lpszProxyBypass) {
            result.exceptions = ieConfig.lpszProxyBypass;
            ::GlobalFree(ieConfig.lpszProxyBypass);
        }
        if (ieConfig.lpszAutoConfigUrl) {
            ::GlobalFree(ieConfig.lpszAutoConfigUrl);
        }
    }
    
    return result;
}

bool ProxyManager::IsException(const String& host) const {
    if (m_config.exceptions.empty()) return false;
    
    auto exceptions = Unicode::Split(m_config.exceptions, L';');
    for (const auto& exc : exceptions) {
        if (Unicode::WildcardMatch(host, Unicode::Trim(exc))) {
            return true;
        }
    }
    return false;
}

} // namespace idm
