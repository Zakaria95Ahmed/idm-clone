/**
 * @file CookieJar.cpp
 */

#include "stdafx.h"
#include "CookieJar.h"
#include "../util/Unicode.h"
#include "../util/Logger.h"

namespace idm {

CookieJar& CookieJar::Instance() {
    static CookieJar instance;
    return instance;
}

void CookieJar::SetCookies(const String& domain, const String& cookies) {
    Lock lock(m_mutex);
    m_cookies[Unicode::ToLower(domain)] = cookies;
}

String CookieJar::GetCookiesForUrl(const String& url) const {
    Lock lock(m_mutex);
    
    String host = Unicode::ToLower(Unicode::ExtractHostFromUrl(url));
    
    // Exact match first
    auto it = m_cookies.find(host);
    if (it != m_cookies.end()) return it->second;
    
    // Try parent domains
    size_t dotPos = host.find(L'.');
    while (dotPos != String::npos) {
        String parent = host.substr(dotPos + 1);
        it = m_cookies.find(parent);
        if (it != m_cookies.end()) return it->second;
        dotPos = host.find(L'.', dotPos + 1);
    }
    
    return L"";
}

void CookieJar::ImportFromBrowser() {
    Lock lock(m_mutex);
    
    // Use WinINet to get cookies from the system cookie store
    // (shared with IE/Edge)
    // For Chrome/Firefox, we'd need to read their cookie databases
    // directly, which is more complex
    
    LOG_INFO(L"CookieJar: import from system cookie store not yet implemented");
}

void CookieJar::Clear() {
    Lock lock(m_mutex);
    m_cookies.clear();
}

} // namespace idm
