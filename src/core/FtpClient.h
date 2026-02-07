/**
 * @file FtpClient.h
 * @brief FTP client supporting active and passive modes
 *
 * Uses WinINet's FTP functions (not WinHTTP, which doesn't support FTP).
 * Supports: login, directory listing, file download, resume via REST command,
 * active and passive modes, proxy tunneling.
 */

#pragma once
#include "stdafx.h"
#include "HttpClient.h"  // For DataCallback

namespace idm {

struct FtpFileInfo {
    String  fileName;
    int64   fileSize{0};
    bool    isDirectory{false};
    FILETIME lastModified{};
};

class FtpClient {
public:
    FtpClient();
    ~FtpClient();
    
    FtpClient(const FtpClient&) = delete;
    FtpClient& operator=(const FtpClient&) = delete;
    
    /**
     * Connect to an FTP server.
     */
    bool Connect(const String& host, uint16 port = 21,
                 const String& username = L"anonymous",
                 const String& password = L"anonymous@",
                 bool passiveMode = true);
    
    /**
     * Disconnect from the server.
     */
    void Disconnect();
    
    /**
     * Get file information (size, date).
     */
    bool GetFileInfo(const String& remotePath, FtpFileInfo& info);
    
    /**
     * Download a file with optional resume position.
     */
    bool Download(const String& remotePath, int64 startPosition,
                  DataCallback callback);
    
    /**
     * List directory contents.
     */
    bool ListDirectory(const String& remotePath, std::vector<FtpFileInfo>& files);
    
    /**
     * Check if connected.
     */
    bool IsConnected() const { return m_hConnection != nullptr; }
    
    void Cancel() { m_cancelled.store(true); }
    String GetLastErrorMessage() const { return m_lastError; }
    
private:
    HINTERNET           m_hInternet{nullptr};
    HINTERNET           m_hConnection{nullptr};
    String              m_lastError;
    std::atomic<bool>   m_cancelled{false};
};

} // namespace idm
