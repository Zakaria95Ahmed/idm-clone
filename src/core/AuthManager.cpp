/**
 * @file AuthManager.cpp
 */

#include "stdafx.h"
#include "AuthManager.h"
#include "../util/Unicode.h"
#include "../util/Registry.h"

namespace idm {

AuthManager& AuthManager::Instance() {
    static AuthManager instance;
    return instance;
}

void AuthManager::AddCredential(const SiteCredential& cred) {
    Lock lock(m_mutex);
    m_credentials.push_back(cred);
    Save();
}

void AuthManager::RemoveCredential(int index) {
    Lock lock(m_mutex);
    if (index >= 0 && index < static_cast<int>(m_credentials.size())) {
        m_credentials.erase(m_credentials.begin() + index);
        Save();
    }
}

void AuthManager::UpdateCredential(int index, const SiteCredential& cred) {
    Lock lock(m_mutex);
    if (index >= 0 && index < static_cast<int>(m_credentials.size())) {
        m_credentials[index] = cred;
        Save();
    }
}

std::optional<SiteCredential> AuthManager::FindCredential(const String& url) const {
    Lock lock(m_mutex);
    for (const auto& cred : m_credentials) {
        if (Unicode::WildcardMatch(url, cred.urlPattern)) {
            return cred;
        }
    }
    return std::nullopt;
}

std::vector<SiteCredential> AuthManager::GetAllCredentials() const {
    Lock lock(m_mutex);
    return m_credentials;
}

void AuthManager::Load() {
    Lock lock(m_mutex);
    m_credentials.clear();
    
    auto& reg = Registry::Instance();
    int count = static_cast<int>(reg.ReadInt(L"SiteLogins", L"Count", 0));
    
    for (int i = 0; i < count; ++i) {
        String prefix = L"SiteLogins\\" + std::to_wstring(i);
        SiteCredential cred;
        cred.urlPattern = reg.ReadString(prefix, L"URL");
        cred.username = reg.ReadString(prefix, L"User");
        cred.password = reg.ReadString(prefix, L"Pass");
        if (!cred.urlPattern.empty()) {
            m_credentials.push_back(cred);
        }
    }
}

void AuthManager::Save() {
    auto& reg = Registry::Instance();
    reg.WriteInt(L"SiteLogins", L"Count", static_cast<DWORD>(m_credentials.size()));
    
    for (int i = 0; i < static_cast<int>(m_credentials.size()); ++i) {
        String prefix = L"SiteLogins\\" + std::to_wstring(i);
        reg.WriteString(prefix, L"URL", m_credentials[i].urlPattern);
        reg.WriteString(prefix, L"User", m_credentials[i].username);
        reg.WriteString(prefix, L"Pass", m_credentials[i].password);
    }
}

} // namespace idm
