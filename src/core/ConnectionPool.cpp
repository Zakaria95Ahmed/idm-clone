/**
 * @file ConnectionPool.cpp
 * @brief Connection pool implementation
 */

#include "stdafx.h"
#include "ConnectionPool.h"
#include "../util/Logger.h"

namespace idm {

ConnectionPool& ConnectionPool::Instance() {
    static ConnectionPool instance;
    return instance;
}

std::unique_ptr<HttpClient> ConnectionPool::AcquireHttpClient() {
    Lock lock(m_httpMutex);
    
    if (!m_httpPool.empty()) {
        auto client = std::move(m_httpPool.back());
        m_httpPool.pop_back();
        client->Reset();
        return client;
    }
    
    return std::make_unique<HttpClient>();
}

void ConnectionPool::ReleaseHttpClient(std::unique_ptr<HttpClient> client) {
    if (!client) return;
    
    Lock lock(m_httpMutex);
    if (static_cast<int>(m_httpPool.size()) < m_maxPoolSize) {
        client->Reset();
        m_httpPool.push_back(std::move(client));
    }
    // Otherwise, let it destruct (close WinHTTP handles)
}

std::unique_ptr<FtpClient> ConnectionPool::AcquireFtpClient() {
    Lock lock(m_ftpMutex);
    
    if (!m_ftpPool.empty()) {
        auto client = std::move(m_ftpPool.back());
        m_ftpPool.pop_back();
        return client;
    }
    
    return std::make_unique<FtpClient>();
}

void ConnectionPool::ReleaseFtpClient(std::unique_ptr<FtpClient> client) {
    if (!client) return;
    
    Lock lock(m_ftpMutex);
    if (static_cast<int>(m_ftpPool.size()) < m_maxPoolSize) {
        m_ftpPool.push_back(std::move(client));
    }
}

void ConnectionPool::Clear() {
    {
        Lock lock(m_httpMutex);
        m_httpPool.clear();
    }
    {
        Lock lock(m_ftpMutex);
        m_ftpPool.clear();
    }
    LOG_DEBUG(L"ConnectionPool: cleared all pooled connections");
}

int ConnectionPool::GetHttpPoolSize() const {
    Lock lock(m_httpMutex);
    return static_cast<int>(m_httpPool.size());
}

int ConnectionPool::GetFtpPoolSize() const {
    Lock lock(m_ftpMutex);
    return static_cast<int>(m_ftpPool.size());
}

} // namespace idm
