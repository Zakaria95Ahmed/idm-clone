/**
 * @file CookieJar.h / CookieJar.cpp
 * @brief Cookie management and browser cookie import
 */

#pragma once
#include "stdafx.h"

namespace idm {

class CookieJar {
public:
    static CookieJar& Instance();
    
    // Set cookies for a domain
    void SetCookies(const String& domain, const String& cookies);
    
    // Get cookies for a URL
    String GetCookiesForUrl(const String& url) const;
    
    // Import cookies from browser (via WinINet cache)
    void ImportFromBrowser();
    
    // Clear all cookies
    void Clear();
    
private:
    CookieJar() = default;
    std::map<String, String> m_cookies;  // domain -> cookie string
    mutable Mutex m_mutex;
};

} // namespace idm
