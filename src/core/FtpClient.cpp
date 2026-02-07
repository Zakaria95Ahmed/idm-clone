/**
 * @file FtpClient.cpp
 * @brief FTP client implementation using WinINet
 */

#include "stdafx.h"
#include "FtpClient.h"
#include "../util/Logger.h"

namespace idm {

FtpClient::FtpClient() {
    m_hInternet = ::InternetOpenW(constants::DEFAULT_USER_AGENT,
        INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
    if (!m_hInternet) {
        m_lastError = L"Failed to initialize WinINet";
    }
}

FtpClient::~FtpClient() {
    Disconnect();
    if (m_hInternet) {
        ::InternetCloseHandle(m_hInternet);
    }
}

bool FtpClient::Connect(const String& host, uint16 port,
                         const String& username, const String& password,
                         bool passiveMode) {
    Disconnect();
    
    DWORD flags = passiveMode ? INTERNET_FLAG_PASSIVE : 0;
    
    m_hConnection = ::InternetConnectW(m_hInternet, host.c_str(), port,
        username.c_str(), password.c_str(),
        INTERNET_SERVICE_FTP, flags, 0);
    
    if (!m_hConnection) {
        m_lastError = L"FTP connection failed to " + host;
        LOG_ERROR(L"FTP: %s (error %lu)", m_lastError.c_str(), ::GetLastError());
        return false;
    }
    
    LOG_INFO(L"FTP: Connected to %s:%d", host.c_str(), port);
    return true;
}

void FtpClient::Disconnect() {
    if (m_hConnection) {
        ::InternetCloseHandle(m_hConnection);
        m_hConnection = nullptr;
    }
}

bool FtpClient::GetFileInfo(const String& remotePath, FtpFileInfo& info) {
    if (!m_hConnection) return false;
    
    WIN32_FIND_DATAW findData = {};
    HINTERNET hFind = ::FtpFindFirstFileW(m_hConnection, remotePath.c_str(),
        &findData, INTERNET_FLAG_RELOAD, 0);
    
    if (!hFind) {
        m_lastError = L"File not found: " + remotePath;
        return false;
    }
    
    info.fileName = findData.cFileName;
    info.fileSize = (static_cast<int64>(findData.nFileSizeHigh) << 32) | findData.nFileSizeLow;
    info.isDirectory = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    info.lastModified = findData.ftLastWriteTime;
    
    ::InternetCloseHandle(hFind);
    return true;
}

bool FtpClient::Download(const String& remotePath, int64 startPosition,
                          DataCallback callback) {
    if (!m_hConnection || !callback) return false;
    
    DWORD flags = FTP_TRANSFER_TYPE_BINARY | INTERNET_FLAG_RELOAD;
    
    HINTERNET hFile = ::FtpOpenFileW(m_hConnection, remotePath.c_str(),
        GENERIC_READ, flags, 0);
    
    if (!hFile) {
        m_lastError = L"Failed to open FTP file: " + remotePath;
        LOG_ERROR(L"FTP: %s (error %lu)", m_lastError.c_str(), ::GetLastError());
        return false;
    }
    
    // Seek to resume position if specified
    if (startPosition > 0) {
        LONG highPart = static_cast<LONG>(startPosition >> 32);
        DWORD result = ::InternetSetFilePointer(hFile, 
            static_cast<LONG>(startPosition & 0xFFFFFFFF),
            &highPart, FILE_BEGIN, 0);
        
        if (result == INVALID_SET_FILE_POINTER && ::GetLastError() != NO_ERROR) {
            LOG_WARN(L"FTP: Resume not supported, starting from beginning");
            // Continue from start
        }
    }
    
    // Read data
    std::vector<uint8> buffer(constants::BUFFER_SIZE);
    DWORD bytesRead = 0;
    
    while (!m_cancelled.load()) {
        if (!::InternetReadFile(hFile, buffer.data(), 
                                static_cast<DWORD>(buffer.size()), &bytesRead)) {
            m_lastError = L"FTP read error";
            ::InternetCloseHandle(hFile);
            return false;
        }
        
        if (bytesRead == 0) break;  // EOF
        
        if (!callback(buffer.data(), bytesRead)) {
            ::InternetCloseHandle(hFile);
            return false;
        }
    }
    
    ::InternetCloseHandle(hFile);
    return !m_cancelled.load();
}

bool FtpClient::ListDirectory(const String& remotePath, std::vector<FtpFileInfo>& files) {
    if (!m_hConnection) return false;
    
    files.clear();
    WIN32_FIND_DATAW findData = {};
    
    HINTERNET hFind = ::FtpFindFirstFileW(m_hConnection, remotePath.c_str(),
        &findData, INTERNET_FLAG_RELOAD, 0);
    
    if (!hFind) return false;
    
    do {
        FtpFileInfo info;
        info.fileName = findData.cFileName;
        info.fileSize = (static_cast<int64>(findData.nFileSizeHigh) << 32) | findData.nFileSizeLow;
        info.isDirectory = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        info.lastModified = findData.ftLastWriteTime;
        files.push_back(std::move(info));
    } while (::InternetFindNextFileW(hFind, &findData));
    
    ::InternetCloseHandle(hFind);
    return true;
}

} // namespace idm
