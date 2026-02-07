/**
 * @file AuthManager.h / AuthManager.cpp
 * @brief Authentication credential manager for site logins
 */

#pragma once
#include "stdafx.h"

namespace idm {

struct SiteCredential {
    String  urlPattern;  // Wildcard pattern (e.g., "*.example.com/*")
    String  username;
    String  password;
};

class AuthManager {
public:
    static AuthManager& Instance();
    
    void AddCredential(const SiteCredential& cred);
    void RemoveCredential(int index);
    void UpdateCredential(int index, const SiteCredential& cred);
    
    // Find matching credentials for a URL
    std::optional<SiteCredential> FindCredential(const String& url) const;
    
    std::vector<SiteCredential> GetAllCredentials() const;
    
    void Load();  // Load from registry
    void Save();  // Save to registry
    
private:
    AuthManager() = default;
    std::vector<SiteCredential> m_credentials;
    mutable Mutex m_mutex;
};

} // namespace idm
