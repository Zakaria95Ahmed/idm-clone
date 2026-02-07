/**
 * @file ConnectionPool.h
 * @brief Connection pool managing concurrent download connections
 */

#pragma once
#include "stdafx.h"
#include "HttpClient.h"
#include "FtpClient.h"

namespace idm {

class ConnectionPool {
public:
    static ConnectionPool& Instance();
    
    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;
    
    /**
     * Get an HTTP client from the pool. Creates a new one if none available.
     */
    std::unique_ptr<HttpClient> AcquireHttpClient();
    
    /**
     * Return an HTTP client to the pool for reuse.
     */
    void ReleaseHttpClient(std::unique_ptr<HttpClient> client);
    
    /**
     * Get an FTP client from the pool.
     */
    std::unique_ptr<FtpClient> AcquireFtpClient();
    void ReleaseFtpClient(std::unique_ptr<FtpClient> client);
    
    /**
     * Set the maximum pool size.
     */
    void SetMaxPoolSize(int size) { m_maxPoolSize = size; }
    
    /**
     * Clear all pooled connections.
     */
    void Clear();
    
    /**
     * Get pool statistics.
     */
    int GetHttpPoolSize() const;
    int GetFtpPoolSize() const;
    
private:
    ConnectionPool() = default;
    ~ConnectionPool() = default;
    
    mutable Mutex   m_httpMutex;
    mutable Mutex   m_ftpMutex;
    std::vector<std::unique_ptr<HttpClient>>  m_httpPool;
    std::vector<std::unique_ptr<FtpClient>>   m_ftpPool;
    int             m_maxPoolSize{64};
};

} // namespace idm
