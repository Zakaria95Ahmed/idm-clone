/**
 * @file ResumeEngine.cpp
 * @brief Resume engine implementation
 */

#include "stdafx.h"
#include "ResumeEngine.h"
#include "SegmentManager.h"
#include "HttpClient.h"
#include "../util/Logger.h"
#include "../util/Unicode.h"

namespace idm {

bool ResumeEngine::ValidateResume(HttpClient& client, DownloadEntry& entry) {
    // Send HEAD request to check if file has changed
    HttpRequestConfig config;
    config.url = entry.url;
    config.userAgent = entry.userAgent.empty() ? constants::DEFAULT_USER_AGENT : entry.userAgent;
    config.referrer = entry.referrer;
    config.cookies = entry.cookies;
    
    HttpResponseInfo response;
    if (!client.Head(config, response)) {
        LOG_WARN(L"ResumeEngine: HEAD request failed for validation");
        return false;
    }
    
    // Check if server supports range requests
    if (!response.acceptRanges) {
        LOG_WARN(L"ResumeEngine: server does not support range requests");
        entry.resumeSupported = false;
        return false;
    }
    
    // Validate ETag
    if (!entry.etag.empty() && !response.etag.empty()) {
        if (entry.etag != response.etag) {
            LOG_WARN(L"ResumeEngine: ETag mismatch (old: %s, new: %s)",
                     entry.etag.c_str(), response.etag.c_str());
            return false;  // File changed on server
        }
    }
    
    // Validate Last-Modified
    if (!entry.lastModified.empty() && !response.lastModified.empty()) {
        if (entry.lastModified != response.lastModified) {
            LOG_WARN(L"ResumeEngine: Last-Modified mismatch");
            return false;  // File changed on server
        }
    }
    
    // Validate Content-Length
    if (entry.fileSize > 0 && response.contentLength > 0 &&
        entry.fileSize != response.contentLength) {
        LOG_WARN(L"ResumeEngine: file size changed (old: %lld, new: %lld)",
                 entry.fileSize, response.contentLength);
        return false;
    }
    
    // Update entry with latest server info
    if (!response.etag.empty()) entry.etag = response.etag;
    if (!response.lastModified.empty()) entry.lastModified = response.lastModified;
    if (response.contentLength > 0) entry.fileSize = response.contentLength;
    entry.resumeSupported = true;
    
    LOG_INFO(L"ResumeEngine: resume validated for %s", entry.fileName.c_str());
    return true;
}

bool ResumeEngine::SaveState(const DownloadEntry& entry, const SegmentManager& segments) {
    // Save segment state to binary file
    String segPath = entry.SegmentPath();
    return segments.SaveState(segPath);
}

bool ResumeEngine::RestoreState(DownloadEntry& entry, SegmentManager& segments) {
    String segPath = entry.SegmentPath();
    
    if (!std::filesystem::exists(segPath)) {
        LOG_DEBUG(L"ResumeEngine: no state file found for %s", entry.fileName.c_str());
        return false;
    }
    
    if (segments.LoadStateFromFile(segPath)) {
        entry.downloadedBytes = segments.GetTotalDownloaded();
        LOG_INFO(L"ResumeEngine: restored state for %s (%lld bytes downloaded)",
                 entry.fileName.c_str(), entry.downloadedBytes);
        return true;
    }
    
    return false;
}

void ResumeEngine::CleanupPartialFiles(const DownloadEntry& entry) {
    std::error_code ec;
    
    // Remove the partial download file
    std::filesystem::remove(entry.PartialPath(), ec);
    
    // Remove the segment state file
    std::filesystem::remove(entry.SegmentPath(), ec);
    
    LOG_DEBUG(L"ResumeEngine: cleaned up partial files for %s", entry.fileName.c_str());
}

bool ResumeEngine::HasPartialFiles(const DownloadEntry& entry) {
    return std::filesystem::exists(entry.PartialPath()) ||
           std::filesystem::exists(entry.SegmentPath());
}

int ResumeEngine::GetRetryDelay(int retryCount, int baseDelaySec) {
    // Exponential backoff: base * 2^(n-1), capped at 300 seconds (5 minutes)
    int delay = baseDelaySec;
    for (int i = 1; i < retryCount && i < 8; ++i) {
        delay *= 2;
    }
    return (std::min)(delay, 300);
}

bool ResumeEngine::ShouldRetry(DWORD errorCode, int httpStatusCode) {
    // Network errors: always retry
    if (errorCode == ERROR_WINHTTP_TIMEOUT ||
        errorCode == ERROR_WINHTTP_CONNECTION_ERROR ||
        errorCode == ERROR_WINHTTP_CANNOT_CONNECT ||
        errorCode == ERROR_WINHTTP_NAME_NOT_RESOLVED) {
        return true;
    }
    
    // HTTP status codes
    switch (httpStatusCode) {
        case 408:  // Request Timeout
        case 429:  // Too Many Requests
        case 500:  // Internal Server Error
        case 502:  // Bad Gateway
        case 503:  // Service Unavailable
        case 504:  // Gateway Timeout
        case 509:  // Bandwidth Limit Exceeded
            return true;
            
        case 401:  // Unauthorized - need credentials
        case 403:  // Forbidden
        case 404:  // Not Found
        case 410:  // Gone
            return false;
            
        default:
            return errorCode != 0;  // Retry on network errors
    }
}

} // namespace idm
