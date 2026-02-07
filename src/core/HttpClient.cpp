/**
 * @file HttpClient.cpp
 * @brief WinHTTP-based HTTP/HTTPS client implementation
 *
 * Implementation notes:
 * - Each request creates fresh connect/request handles to avoid state pollution
 * - The session handle (HINTERNET from WinHttpOpen) is created once per client
 *   instance and reused across requests for connection pooling benefits
 * - WinHTTP internally manages connection keep-alive and pipelining
 * - Redirect following is done manually to capture intermediate URLs
 * - Range header is set explicitly for segmented downloads
 *
 * Error handling: WinHTTP errors are captured via GetLastError() and
 * translated to human-readable messages stored in m_lastError.
 */

#include "stdafx.h"
#include "HttpClient.h"
#include "../util/Logger.h"
#include "../util/Unicode.h"

namespace idm {

// ─── Construction / Destruction ────────────────────────────────────────────
HttpClient::HttpClient() {
    // Create WinHTTP session handle
    // WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY: use system proxy by default
    // Individual requests can override this with explicit proxy settings
    m_hSession = ::WinHttpOpen(
        constants::DEFAULT_USER_AGENT,
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0  // Synchronous mode (async handled at a higher level)
    );
    
    if (m_hSession) {
        // Enable HTTP/2 for better performance on modern servers
        DWORD http2 = WINHTTP_PROTOCOL_FLAG_HTTP2;
        ::WinHttpSetOption(m_hSession, WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL,
                          &http2, sizeof(http2));
        
        // Set reasonable default timeouts
        // WinHttpSetTimeouts(session, resolve, connect, send, receive)
        ::WinHttpSetTimeouts(m_hSession, 10000, 30000, 30000, 60000);
    } else {
        SetError(L"Failed to create WinHTTP session", ::GetLastError());
    }
}

HttpClient::~HttpClient() {
    CleanupHandles();
    if (m_hSession) {
        ::WinHttpCloseHandle(m_hSession);
        m_hSession = nullptr;
    }
}

// Move operations
HttpClient::HttpClient(HttpClient&& other) noexcept 
    : m_hSession(other.m_hSession)
    , m_hConnect(other.m_hConnect)
    , m_hRequest(other.m_hRequest)
    , m_lastError(std::move(other.m_lastError))
    , m_lastErrorCode(other.m_lastErrorCode)
{
    m_cancelled.store(other.m_cancelled.load());
    other.m_hSession = nullptr;
    other.m_hConnect = nullptr;
    other.m_hRequest = nullptr;
}

HttpClient& HttpClient::operator=(HttpClient&& other) noexcept {
    if (this != &other) {
        CleanupHandles();
        if (m_hSession) ::WinHttpCloseHandle(m_hSession);
        
        m_hSession = other.m_hSession;
        m_hConnect = other.m_hConnect;
        m_hRequest = other.m_hRequest;
        m_lastError = std::move(other.m_lastError);
        m_lastErrorCode = other.m_lastErrorCode;
        m_cancelled.store(other.m_cancelled.load());
        
        other.m_hSession = nullptr;
        other.m_hConnect = nullptr;
        other.m_hRequest = nullptr;
    }
    return *this;
}

// ─── Public Methods ────────────────────────────────────────────────────────
bool HttpClient::Head(const HttpRequestConfig& config, HttpResponseInfo& response) {
    HttpRequestConfig headConfig = config;
    headConfig.method = L"HEAD";
    return ExecuteRequest(headConfig, response, nullptr);
}

bool HttpClient::Get(const HttpRequestConfig& config, HttpResponseInfo& response,
                     DataCallback callback) {
    HttpRequestConfig getConfig = config;
    getConfig.method = L"GET";
    return ExecuteRequest(getConfig, response, callback);
}

bool HttpClient::Post(const HttpRequestConfig& config, HttpResponseInfo& response,
                      DataCallback callback) {
    HttpRequestConfig postConfig = config;
    postConfig.method = L"POST";
    return ExecuteRequest(postConfig, response, callback);
}

void HttpClient::Cancel() {
    m_cancelled.store(true);
    // Close the request handle to unblock any pending I/O
    if (m_hRequest) {
        ::WinHttpCloseHandle(m_hRequest);
        m_hRequest = nullptr;
    }
}

void HttpClient::Reset() {
    m_cancelled.store(false);
    m_lastError.clear();
    m_lastErrorCode = 0;
}

// ─── Core Request Execution ────────────────────────────────────────────────
bool HttpClient::ExecuteRequest(const HttpRequestConfig& config,
                                 HttpResponseInfo& response,
                                 DataCallback callback) {
    if (!m_hSession) {
        SetError(L"WinHTTP session not initialized");
        return false;
    }
    
    if (m_cancelled.load()) {
        SetError(L"Request cancelled");
        return false;
    }
    
    // Clean up any previous request handles
    CleanupHandles();
    
    // Parse URL components
    URL_COMPONENTS urlComponents = {};
    urlComponents.dwStructSize = sizeof(URL_COMPONENTS);
    
    wchar_t hostName[256] = {};
    wchar_t urlPath[4096] = {};
    urlComponents.lpszHostName = hostName;
    urlComponents.dwHostNameLength = _countof(hostName);
    urlComponents.lpszUrlPath = urlPath;
    urlComponents.dwUrlPathLength = _countof(urlPath);
    
    if (!::WinHttpCrackUrl(config.url.c_str(), 
                           static_cast<DWORD>(config.url.length()), 0, &urlComponents)) {
        SetError(L"Failed to parse URL: " + config.url, ::GetLastError());
        return false;
    }
    
    bool isSecure = (urlComponents.nScheme == INTERNET_SCHEME_HTTPS);
    INTERNET_PORT port = urlComponents.nPort;
    
    response.finalUrl = config.url;
    
    // Set proxy if configured
    if (!config.proxyAddr.empty()) {
        WINHTTP_PROXY_INFO proxyInfo = {};
        proxyInfo.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
        proxyInfo.lpszProxy = const_cast<LPWSTR>(config.proxyAddr.c_str());
        ::WinHttpSetOption(m_hSession, WINHTTP_OPTION_PROXY,
                          &proxyInfo, sizeof(proxyInfo));
    }
    
    // Open connection to the server
    m_hConnect = ::WinHttpConnect(m_hSession, hostName, port, 0);
    if (!m_hConnect) {
        SetError(L"Failed to connect to " + String(hostName), ::GetLastError());
        return false;
    }
    
    // Create HTTP request
    DWORD flags = isSecure ? WINHTTP_FLAG_SECURE : 0;
    
    m_hRequest = ::WinHttpOpenRequest(
        m_hConnect,
        config.method.c_str(),
        urlPath,
        nullptr,                        // HTTP/1.1
        config.referrer.empty() ? WINHTTP_NO_REFERER : config.referrer.c_str(),
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags
    );
    
    if (!m_hRequest) {
        SetError(L"Failed to create HTTP request", ::GetLastError());
        return false;
    }
    
    // Configure timeouts for this request
    ::WinHttpSetTimeouts(m_hRequest,
        10000,                                          // DNS resolve
        config.timeoutConnect * 1000,                   // Connect
        config.timeoutConnect * 1000,                   // Send
        config.timeoutReceive * 1000);                  // Receive
    
    // Disable auto-redirect to handle it manually (capture intermediate URLs)
    DWORD disableRedirect = WINHTTP_DISABLE_REDIRECTS;
    ::WinHttpSetOption(m_hRequest, WINHTTP_OPTION_DISABLE_FEATURE,
                      &disableRedirect, sizeof(disableRedirect));
    
    // SSL verification
    if (isSecure && !config.verifySSL) {
        DWORD secFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                         SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                         SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                         SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
        ::WinHttpSetOption(m_hRequest, WINHTTP_OPTION_SECURITY_FLAGS,
                          &secFlags, sizeof(secFlags));
    }
    
    // Build custom headers
    String headers;
    
    // User-Agent (override WinHTTP default if specified)
    if (!config.userAgent.empty()) {
        headers += L"User-Agent: " + config.userAgent + L"\r\n";
    }
    
    // Cookies
    if (!config.cookies.empty()) {
        headers += L"Cookie: " + config.cookies + L"\r\n";
    }
    
    // Range header for segmented downloads
    String rangeHeader = config.GetRangeHeader();
    if (!rangeHeader.empty()) {
        headers += L"Range: " + rangeHeader + L"\r\n";
    }
    
    // Custom headers from config
    for (const auto& [name, value] : config.customHeaders) {
        headers += name + L": " + value + L"\r\n";
    }
    
    // Add headers to request
    if (!headers.empty()) {
        ::WinHttpAddRequestHeaders(m_hRequest, headers.c_str(),
            static_cast<DWORD>(headers.length()), WINHTTP_ADDREQ_FLAG_ADD);
    }
    
    // Set authentication if provided
    if (!config.username.empty()) {
        ::WinHttpSetCredentials(m_hRequest, WINHTTP_AUTH_TARGET_SERVER,
            WINHTTP_AUTH_SCHEME_BASIC,
            config.username.c_str(), config.password.c_str(), nullptr);
    }
    
    // Proxy authentication
    if (!config.proxyUsername.empty()) {
        ::WinHttpSetCredentials(m_hRequest, WINHTTP_AUTH_TARGET_PROXY,
            WINHTTP_AUTH_SCHEME_BASIC,
            config.proxyUsername.c_str(), config.proxyPassword.c_str(), nullptr);
    }
    
    // Send the request
    LPVOID postBody = nullptr;
    DWORD postBodyLen = 0;
    std::string postDataUtf8;
    
    if (!config.postData.empty()) {
        postDataUtf8 = Unicode::WideToUtf8(config.postData);
        postBody = const_cast<char*>(postDataUtf8.c_str());
        postBodyLen = static_cast<DWORD>(postDataUtf8.length());
    }
    
    if (!::WinHttpSendRequest(m_hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                               postBody, postBodyLen, postBodyLen, 0)) {
        DWORD err = ::GetLastError();
        if (m_cancelled.load()) {
            SetError(L"Request cancelled");
        } else {
            SetError(L"Failed to send request", err);
        }
        return false;
    }
    
    // Receive response
    if (!::WinHttpReceiveResponse(m_hRequest, nullptr)) {
        DWORD err = ::GetLastError();
        if (m_cancelled.load()) {
            SetError(L"Request cancelled");
        } else {
            SetError(L"Failed to receive response", err);
        }
        return false;
    }
    
    // Parse response headers
    ParseResponseHeaders(m_hRequest, response);
    
    // Handle redirects manually
    int redirectCount = 0;
    while ((response.statusCode == 301 || response.statusCode == 302 ||
            response.statusCode == 307 || response.statusCode == 308) &&
           !response.location.empty() &&
           redirectCount < config.maxRedirects) {
        
        LOG_DEBUG(L"Redirect %d -> %s", response.statusCode, response.location.c_str());
        
        response.finalUrl = response.location;
        redirectCount++;
        
        // Recursive call with new URL
        HttpRequestConfig redirectConfig = config;
        redirectConfig.url = response.location;
        redirectConfig.maxRedirects = config.maxRedirects - redirectCount;
        
        // For 307/308, preserve method; for 301/302, change to GET
        if (response.statusCode == 301 || response.statusCode == 302) {
            redirectConfig.method = L"GET";
            redirectConfig.postData.clear();
        }
        
        CleanupHandles();
        return ExecuteRequest(redirectConfig, response, callback);
    }
    
    // Read response body if callback provided and method is not HEAD
    if (callback && config.method != L"HEAD") {
        std::vector<uint8> buffer(constants::BUFFER_SIZE);
        DWORD bytesAvailable = 0;
        DWORD bytesRead = 0;
        
        while (!m_cancelled.load()) {
            if (!::WinHttpQueryDataAvailable(m_hRequest, &bytesAvailable)) {
                if (!m_cancelled.load()) {
                    SetError(L"Failed to query data availability", ::GetLastError());
                }
                return false;
            }
            
            if (bytesAvailable == 0) break;  // End of data
            
            // Read in chunks up to buffer size
            DWORD toRead = min(bytesAvailable, static_cast<DWORD>(buffer.size()));
            
            if (!::WinHttpReadData(m_hRequest, buffer.data(), toRead, &bytesRead)) {
                if (!m_cancelled.load()) {
                    SetError(L"Failed to read data", ::GetLastError());
                }
                return false;
            }
            
            if (bytesRead == 0) break;  // Connection closed
            
            // Deliver data to callback
            if (!callback(buffer.data(), bytesRead)) {
                SetError(L"Download cancelled by callback");
                return false;
            }
        }
    }
    
    if (m_cancelled.load()) {
        SetError(L"Request cancelled");
        return false;
    }
    
    return true;
}

// ─── Parse Response Headers ────────────────────────────────────────────────
void HttpClient::ParseResponseHeaders(HINTERNET hRequest, HttpResponseInfo& response) {
    // Status code
    DWORD statusCode = 0;
    DWORD size = sizeof(DWORD);
    ::WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                         WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &size, 
                         WINHTTP_NO_HEADER_INDEX);
    response.statusCode = static_cast<int>(statusCode);
    
    // Status text
    response.statusText = QueryResponseHeader(hRequest, WINHTTP_QUERY_STATUS_TEXT);
    
    // Content-Length
    String contentLengthStr = QueryResponseHeader(hRequest, WINHTTP_QUERY_CONTENT_LENGTH);
    if (!contentLengthStr.empty()) {
        try { response.contentLength = std::stoll(contentLengthStr); }
        catch (...) { response.contentLength = -1; }
    }
    
    // Content-Type
    response.contentType = QueryResponseHeader(hRequest, WINHTTP_QUERY_CONTENT_TYPE);
    
    // Content-Disposition  
    response.contentDisposition = QueryResponseHeader(hRequest, WINHTTP_QUERY_CONTENT_DISPOSITION);
    
    // Accept-Ranges
    String acceptRanges = QueryResponseHeader(hRequest, WINHTTP_QUERY_ACCEPT_RANGES);
    response.acceptRanges = !acceptRanges.empty() && acceptRanges != L"none";
    
    // ETag
    response.etag = QueryResponseHeader(hRequest, WINHTTP_QUERY_ETAG);
    
    // Last-Modified
    response.lastModified = QueryResponseHeader(hRequest, WINHTTP_QUERY_LAST_MODIFIED);
    
    // Location (for redirects)
    response.location = QueryResponseHeader(hRequest, WINHTTP_QUERY_LOCATION);
    
    // Resolve relative redirect URLs
    if (!response.location.empty() && response.location.find(L"://") == String::npos) {
        // Relative URL - resolve against the request URL
        if (response.location[0] == L'/') {
            // Absolute path
            String baseUrl = response.finalUrl;
            auto protoEnd = baseUrl.find(L"://");
            if (protoEnd != String::npos) {
                auto pathStart = baseUrl.find(L'/', protoEnd + 3);
                if (pathStart != String::npos) {
                    response.location = baseUrl.substr(0, pathStart) + response.location;
                }
            }
        }
    }
    
    // Get all headers as raw text for the headers map
    DWORD headersSize = 0;
    ::WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF,
                         WINHTTP_HEADER_NAME_BY_INDEX, nullptr, &headersSize,
                         WINHTTP_NO_HEADER_INDEX);
    
    if (headersSize > 0) {
        String rawHeaders(headersSize / sizeof(wchar_t) + 1, L'\0');
        ::WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF,
                             WINHTTP_HEADER_NAME_BY_INDEX, &rawHeaders[0], &headersSize,
                             WINHTTP_NO_HEADER_INDEX);
        
        // Parse into map
        std::wistringstream stream(rawHeaders);
        String line;
        while (std::getline(stream, line)) {
            // Remove \r
            if (!line.empty() && line.back() == L'\r') line.pop_back();
            
            auto colonPos = line.find(L':');
            if (colonPos != String::npos) {
                String name = Unicode::Trim(line.substr(0, colonPos));
                String value = Unicode::Trim(line.substr(colonPos + 1));
                if (!name.empty()) {
                    response.headers[Unicode::ToLower(name)] = value;
                }
            }
        }
    }
}

// ─── Query Single Response Header ──────────────────────────────────────────
String HttpClient::QueryResponseHeader(HINTERNET hRequest, DWORD headerIndex) {
    DWORD size = 0;
    ::WinHttpQueryHeaders(hRequest, headerIndex, WINHTTP_HEADER_NAME_BY_INDEX,
                         nullptr, &size, WINHTTP_NO_HEADER_INDEX);
    
    if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER || size == 0) {
        return L"";
    }
    
    String header(size / sizeof(wchar_t) + 1, L'\0');
    if (::WinHttpQueryHeaders(hRequest, headerIndex, WINHTTP_HEADER_NAME_BY_INDEX,
                              &header[0], &size, WINHTTP_NO_HEADER_INDEX)) {
        header.resize(size / sizeof(wchar_t));
        return header;
    }
    return L"";
}

// ─── Error Handling ────────────────────────────────────────────────────────
void HttpClient::SetError(const String& msg, DWORD code) {
    m_lastError = msg;
    m_lastErrorCode = code;
    
    if (code != 0) {
        wchar_t errBuf[256];
        DWORD len = ::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE,
            ::GetModuleHandleW(L"winhttp.dll"), code, 0, errBuf, 256, nullptr);
        if (len > 0) {
            m_lastError += L" (";
            m_lastError += errBuf;
            // Remove trailing newline
            while (!m_lastError.empty() && 
                   (m_lastError.back() == L'\n' || m_lastError.back() == L'\r')) {
                m_lastError.pop_back();
            }
            m_lastError += L")";
        }
    }
    
    LOG_ERROR(L"HttpClient: %s", m_lastError.c_str());
}

// ─── Handle Cleanup ────────────────────────────────────────────────────────
void HttpClient::CleanupHandles() {
    if (m_hRequest) {
        ::WinHttpCloseHandle(m_hRequest);
        m_hRequest = nullptr;
    }
    if (m_hConnect) {
        ::WinHttpCloseHandle(m_hConnect);
        m_hConnect = nullptr;
    }
}

// ─── Content-Disposition Filename Parsing ──────────────────────────────────
String HttpResponseInfo::GetDispositionFilename() const {
    if (contentDisposition.empty()) return L"";
    
    // Look for: filename="xxx" or filename*=UTF-8''xxx
    String cd = contentDisposition;
    
    // Try filename*= first (RFC 5987 extended notation)
    auto starPos = cd.find(L"filename*=");
    if (starPos != String::npos) {
        String value = cd.substr(starPos + 10);
        // Format: charset'language'filename
        auto tickPos = value.find(L"''");
        if (tickPos != String::npos) {
            String encoded = value.substr(tickPos + 2);
            // Remove trailing semicolons or whitespace
            auto endPos = encoded.find_first_of(L"; \t");
            if (endPos != String::npos) encoded = encoded.substr(0, endPos);
            return Unicode::UrlDecode(encoded);
        }
    }
    
    // Try filename="xxx"
    auto fnPos = cd.find(L"filename=");
    if (fnPos != String::npos) {
        String value = cd.substr(fnPos + 9);
        if (!value.empty() && value[0] == L'"') {
            auto closeQuote = value.find(L'"', 1);
            if (closeQuote != String::npos) {
                return value.substr(1, closeQuote - 1);
            }
        } else {
            auto endPos = value.find_first_of(L"; \t");
            if (endPos != String::npos) value = value.substr(0, endPos);
            return Unicode::Trim(value);
        }
    }
    
    return L"";
}

} // namespace idm
