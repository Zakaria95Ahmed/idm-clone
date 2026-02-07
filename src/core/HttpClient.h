/**
 * @file HttpClient.h
 * @brief HTTP/HTTPS client built on WinHTTP API
 *
 * Architecture: This client wraps WinHTTP (not WinINet) because WinHTTP
 * provides superior control over:
 *   - Per-request proxy configuration
 *   - Server certificate validation callbacks
 *   - Asynchronous operations with overlapped I/O
 *   - Better performance in high-concurrency scenarios
 *
 * The client supports:
 *   - GET, HEAD, POST methods
 *   - Range requests (for segmented downloads)
 *   - All redirect types (301, 302, 307, 308)
 *   - Cookie management (import from browsers)
 *   - All authentication schemes (Basic, Digest, NTLM, Negotiate)
 *   - Custom headers
 *   - SSL/TLS with certificate verification options
 *   - Proxy configuration (HTTP, SOCKS via helper)
 *
 * Thread safety: Each HttpClient instance is independent and can be
 * used from any thread. The WinHTTP session handle is shared via
 * the ConnectionPool for efficiency.
 */

#pragma once
#include "stdafx.h"
#include "../util/Database.h"

namespace idm {

// ─── HTTP Response Headers ─────────────────────────────────────────────────
struct HttpResponseInfo {
    int                 statusCode{0};
    String              statusText;
    int64               contentLength{-1};       // -1 = unknown
    String              contentType;
    String              contentDisposition;
    bool                acceptRanges{false};      // Server supports Range requests
    String              etag;
    String              lastModified;
    String              location;                 // Redirect URL
    String              finalUrl;                 // After all redirects
    std::map<String, String> headers;             // All response headers
    
    // Parse filename from Content-Disposition header
    String GetDispositionFilename() const;
};

// ─── HTTP Request Configuration ────────────────────────────────────────────
struct HttpRequestConfig {
    String              url;
    String              method = L"GET";          // GET, HEAD, POST
    String              referrer;
    String              userAgent;
    String              cookies;
    String              postData;
    String              contentType;              // For POST
    String              username;                 // HTTP auth
    String              password;
    std::map<String, String> customHeaders;
    
    // Range request parameters
    int64               rangeStart{-1};           // -1 = no range
    int64               rangeEnd{-1};             // -1 = to end
    
    // Proxy
    String              proxyAddr;                // host:port
    String              proxyUsername;
    String              proxyPassword;
    
    // Behavior
    int                 maxRedirects{10};
    int                 timeoutConnect{30};       // seconds
    int                 timeoutReceive{60};       // seconds
    bool                verifySSL{true};
    bool                followRedirects{true};
    
    // Build Range header string
    String GetRangeHeader() const {
        if (rangeStart < 0) return L"";
        if (rangeEnd < 0) return L"bytes=" + std::to_wstring(rangeStart) + L"-";
        return L"bytes=" + std::to_wstring(rangeStart) + L"-" + std::to_wstring(rangeEnd);
    }
};

// ─── Data Receive Callback ─────────────────────────────────────────────────
// Called for each chunk of data received. Return false to abort download.
using DataCallback = std::function<bool(const uint8* data, size_t length)>;

// ─── HTTP Client Class ─────────────────────────────────────────────────────
class HttpClient {
public:
    HttpClient();
    ~HttpClient();
    
    // Non-copyable (owns WinHTTP handles)
    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;
    
    // Movable
    HttpClient(HttpClient&& other) noexcept;
    HttpClient& operator=(HttpClient&& other) noexcept;
    
    /**
     * Perform a HEAD request to get file information without downloading.
     * Used to determine: file size, resume support, content type, filename.
     */
    bool Head(const HttpRequestConfig& config, HttpResponseInfo& response);
    
    /**
     * Perform a GET request and receive data via callback.
     * The callback is invoked for each chunk of data received (typically 64KB).
     * Download can be cancelled by returning false from the callback.
     *
     * @param config    Request configuration (URL, headers, range, etc.)
     * @param response  Populated with response headers
     * @param callback  Called for each data chunk
     * @return true if the request completed successfully
     */
    bool Get(const HttpRequestConfig& config, HttpResponseInfo& response,
             DataCallback callback);
    
    /**
     * Perform a POST request.
     */
    bool Post(const HttpRequestConfig& config, HttpResponseInfo& response,
              DataCallback callback);
    
    /**
     * Get the last error message.
     */
    String GetLastErrorMessage() const { return m_lastError; }
    DWORD GetLastErrorCode() const { return m_lastErrorCode; }
    
    /**
     * Cancel an in-progress request (thread-safe).
     */
    void Cancel();
    
    /**
     * Check if the client has been cancelled.
     */
    bool IsCancelled() const { return m_cancelled.load(); }
    
    /**
     * Reset cancelled state for reuse.
     */
    void Reset();
    
private:
    bool ExecuteRequest(const HttpRequestConfig& config, 
                        HttpResponseInfo& response,
                        DataCallback callback);
    
    void ParseResponseHeaders(HINTERNET hRequest, HttpResponseInfo& response);
    String QueryResponseHeader(HINTERNET hRequest, DWORD headerIndex);
    void SetError(const String& msg, DWORD code = 0);
    void CleanupHandles();
    
    HINTERNET           m_hSession{nullptr};
    HINTERNET           m_hConnect{nullptr};
    HINTERNET           m_hRequest{nullptr};
    String              m_lastError;
    DWORD               m_lastErrorCode{0};
    std::atomic<bool>   m_cancelled{false};
};

} // namespace idm
