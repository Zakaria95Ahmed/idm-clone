/**
 * @file DownloadEngine.cpp
 * @brief Main download engine implementation
 *
 * Orchestrates the complete download lifecycle:
 *   1. URL probing (HEAD request for file info)
 *   2. Segmentation initialization
 *   3. Connection spawning (one thread per segment)
 *   4. Progress monitoring and UI notification
 *   5. Error handling and retry logic
 *   6. File assembly and finalization
 *   7. State persistence for crash recovery
 */

#include "stdafx.h"
#include "DownloadEngine.h"
#include "HttpClient.h"
#include "FtpClient.h"
#include "SegmentManager.h"
#include "ResumeEngine.h"
#include "FileAssembler.h"
#include "ConnectionPool.h"
#include "ProxyManager.h"
#include "AuthManager.h"
#include "CookieJar.h"
#include "SpeedLimiter.h"
#include "../util/Logger.h"
#include "../util/Unicode.h"
#include "../util/Registry.h"
#include "../util/Crypto.h"

namespace idm {

// ─── Singleton ─────────────────────────────────────────────────────────────
DownloadEngine& DownloadEngine::Instance() {
    static DownloadEngine instance;
    return instance;
}

DownloadEngine::~DownloadEngine() {
    Shutdown();
}

// ─── Initialize ────────────────────────────────────────────────────────────
bool DownloadEngine::Initialize(const String& dataDir) {
    if (m_initialized) return true;
    
    m_dataDir = dataDir;
    
    // Ensure data directory exists
    std::error_code ec;
    std::filesystem::create_directories(dataDir, ec);
    
    // Open the download database
    String dbPath = dataDir + L"\\downloads.db";
    if (!m_database.Open(dbPath)) {
        LOG_FATAL(L"DownloadEngine: failed to open database at %s", dbPath.c_str());
        return false;
    }
    
    // Load site credentials
    AuthManager::Instance().Load();
    
    // Start background threads
    m_running.store(true);
    
    m_speedMonitor = std::thread(&DownloadEngine::SpeedMonitorThread, this);
    m_statePersist = std::thread(&DownloadEngine::StatePersistThread, this);
    
    m_initialized = true;
    LOG_INFO(L"DownloadEngine: initialized with data dir %s", dataDir.c_str());
    
    return true;
}

// ─── Shutdown ──────────────────────────────────────────────────────────────
void DownloadEngine::Shutdown() {
    if (!m_initialized) return;
    
    LOG_INFO(L"DownloadEngine: shutting down...");
    
    // Stop all downloads
    StopAll();
    
    // Stop background threads
    m_running.store(false);
    
    if (m_speedMonitor.joinable()) m_speedMonitor.join();
    if (m_statePersist.joinable()) m_statePersist.join();
    
    // Save database
    m_database.Flush();
    m_database.Close();
    
    m_initialized = false;
    LOG_INFO(L"DownloadEngine: shutdown complete");
}

// ─── Add Download ──────────────────────────────────────────────────────────
String DownloadEngine::AddDownload(const String& url, const String& savePath,
                                    const String& fileName, const String& referrer,
                                    const String& cookies, bool startImmediately) {
    DownloadEntry entry;
    entry.url = url;
    entry.savePath = savePath;
    entry.fileName = fileName;
    entry.referrer = referrer;
    entry.cookies = cookies;
    
    return AddDownload(entry, startImmediately);
}

String DownloadEngine::AddDownload(DownloadEntry& entry, bool startImmediately) {
    // Generate ID
    entry.id = DownloadEntry::GenerateId();
    entry.dateAdded = SystemClock::now();
    
    // Auto-detect filename from URL if not provided
    if (entry.fileName.empty()) {
        entry.fileName = Unicode::ExtractFilenameFromUrl(entry.url);
    }
    
    // Auto-detect category from extension
    String ext = Unicode::GetFileExtension(entry.fileName);
    if (entry.category.empty()) {
        entry.category = Unicode::CategorizeByExtension(ext);
    }
    
    // Default user agent
    if (entry.userAgent.empty()) {
        entry.userAgent = constants::DEFAULT_USER_AGENT;
    }
    
    // Default save path
    if (entry.savePath.empty()) {
        auto settings = Registry::Instance().LoadSettings();
        entry.savePath = settings.defaultSaveDir;
    }
    
    // Check for stored credentials
    auto cred = AuthManager::Instance().FindCredential(entry.url);
    if (cred.has_value() && entry.username.empty()) {
        entry.username = cred->username;
        entry.password = cred->password;
    }
    
    // Get cookies from jar if not provided
    if (entry.cookies.empty()) {
        entry.cookies = CookieJar::Instance().GetCookiesForUrl(entry.url);
    }
    
    // Set initial status
    entry.status = startImmediately ? DownloadStatus::Connecting : DownloadStatus::Queued;
    
    // Save to database
    m_database.AddEntry(entry);
    
    NotifyAdded(entry.id);
    
    // Start download if requested
    if (startImmediately) {
        StartDownload(entry.id);
    }
    
    LOG_INFO(L"DownloadEngine: added download %s -> %s\\%s",
             entry.url.c_str(), entry.savePath.c_str(), entry.fileName.c_str());
    
    return entry.id;
}

// ─── Start Download ────────────────────────────────────────────────────────
bool DownloadEngine::StartDownload(const String& id) {
    RecursiveLock lock(m_downloadsMutex);
    
    // Check if already active
    if (m_activeDownloads.count(id)) {
        auto& active = m_activeDownloads[id];
        if (active->paused.load()) {
            active->paused.store(false);
            NotifyResumed(id);
            return true;
        }
        return false;  // Already running
    }
    
    // Get entry from database
    auto entryOpt = m_database.GetEntry(id);
    if (!entryOpt.has_value()) {
        LOG_ERROR(L"DownloadEngine: download %s not found", id.c_str());
        return false;
    }
    
    auto entry = entryOpt.value();
    entry.status = DownloadStatus::Connecting;
    m_database.UpdateEntry(entry);
    
    // Create active download
    auto active = std::make_shared<ActiveDownload>();
    active->id = id;
    active->entry = entry;
    active->startTime = Clock::now();
    active->lastStateSave = Clock::now();
    
    m_activeDownloads[id] = active;
    
    // Launch download worker thread
    std::thread worker(&DownloadEngine::DownloadWorker, this, id);
    worker.detach();
    
    NotifyStarted(id);
    return true;
}

// ─── Download Worker Thread ────────────────────────────────────────────────
void DownloadEngine::DownloadWorker(const String& id) {
    LOG_INFO(L"DownloadEngine: download worker started for %s", id.c_str());
    
    std::shared_ptr<ActiveDownload> active;
    {
        RecursiveLock lock(m_downloadsMutex);
        auto it = m_activeDownloads.find(id);
        if (it == m_activeDownloads.end()) return;
        active = it->second;
    }
    
    auto& entry = active->entry;
    
    // Phase 1: Probe the URL (HEAD request)
    auto httpClient = ConnectionPool::Instance().AcquireHttpClient();
    
    HttpRequestConfig probeConfig;
    probeConfig.url = entry.url;
    probeConfig.userAgent = entry.userAgent;
    probeConfig.referrer = entry.referrer;
    probeConfig.cookies = entry.cookies;
    probeConfig.username = entry.username;
    probeConfig.password = entry.password;
    
    // Apply proxy settings
    auto proxy = ProxyManager::Instance().GetProxyForUrl(entry.url);
    if (proxy.type != ProxyType::None) {
        probeConfig.proxyAddr = proxy.address + L":" + std::to_wstring(proxy.port);
        probeConfig.proxyUsername = proxy.username;
        probeConfig.proxyPassword = proxy.password;
    }
    
    HttpResponseInfo probeResponse;
    bool probeOk = httpClient->Head(probeConfig, probeResponse);
    
    if (!probeOk || (probeResponse.statusCode >= 400)) {
        String error = probeOk 
            ? L"HTTP " + std::to_wstring(probeResponse.statusCode) + L" " + probeResponse.statusText
            : httpClient->GetLastErrorMessage();
        
        entry.status = DownloadStatus::Error;
        entry.errorMessage = error;
        m_database.UpdateEntry(entry);
        
        NotifyError(id, error);
        ConnectionPool::Instance().ReleaseHttpClient(std::move(httpClient));
        
        RecursiveLock lock(m_downloadsMutex);
        m_activeDownloads.erase(id);
        return;
    }
    
    // Update entry with server info
    if (probeResponse.contentLength > 0) entry.fileSize = probeResponse.contentLength;
    entry.resumeSupported = probeResponse.acceptRanges;
    entry.etag = probeResponse.etag;
    entry.lastModified = probeResponse.lastModified;
    entry.finalUrl = probeResponse.finalUrl;
    entry.contentType = probeResponse.contentType;
    
    // Determine filename from Content-Disposition if available
    String dispositionName = probeResponse.GetDispositionFilename();
    if (!dispositionName.empty()) {
        entry.fileName = Unicode::SanitizeFilename(dispositionName);
    }
    
    entry.status = DownloadStatus::Downloading;
    m_database.UpdateEntry(entry);
    
    ConnectionPool::Instance().ReleaseHttpClient(std::move(httpClient));
    
    // Phase 2: Initialize segmentation
    auto& segments = active->segments;
    
    // Try to restore from previous session
    bool resumed = false;
    if (entry.downloadedBytes > 0 && entry.resumeSupported) {
        resumed = ResumeEngine::RestoreState(entry, segments);
    }
    
    if (!resumed) {
        int numConns = entry.resumeSupported ? entry.numConnections : 1;
        segments.Initialize(entry.fileSize, numConns);
    }
    
    // Phase 3: Open the partial file
    active->hFile = FileAssembler::OpenPartialFile(entry.PartialPath(), entry.fileSize);
    if (active->hFile == INVALID_HANDLE_VALUE) {
        entry.status = DownloadStatus::Error;
        entry.errorMessage = L"Failed to create download file";
        m_database.UpdateEntry(entry);
        NotifyError(id, entry.errorMessage);
        
        RecursiveLock lock(m_downloadsMutex);
        m_activeDownloads.erase(id);
        return;
    }
    
    // Phase 4: Launch connection threads
    int numConnections = entry.resumeSupported ? 
        (std::min)(entry.numConnections, constants::MAX_CONNECTIONS) : 1;
    
    LOG_INFO(L"DownloadEngine: starting %d connections for %s (%s bytes)",
             numConnections, entry.fileName.c_str(),
             Unicode::FormatFileSize(entry.fileSize).c_str());
    
    // Launch connection workers
    std::vector<std::thread> connThreads;
    for (int i = 0; i < numConnections; ++i) {
        connThreads.emplace_back(&DownloadEngine::ConnectionWorker, this, id, i);
    }
    
    // Wait for all connections to complete
    for (auto& t : connThreads) {
        if (t.joinable()) t.join();
    }
    
    // Close the file handle
    if (active->hFile != INVALID_HANDLE_VALUE) {
        ::CloseHandle(active->hFile);
        active->hFile = INVALID_HANDLE_VALUE;
    }
    
    // Phase 5: Check completion status
    if (active->cancelled.load()) {
        entry.status = DownloadStatus::Paused;
        entry.downloadedBytes = segments.GetTotalDownloaded();
        m_database.UpdateEntry(entry);
        ResumeEngine::SaveState(entry, segments);
        
        NotifyPaused(id);
    } else if (segments.IsComplete()) {
        // Finalize: rename partial to final file
        entry.status = DownloadStatus::Merging;
        m_database.UpdateEntry(entry);
        
        String finalPath = FileAssembler::Finalize(
            entry.PartialPath(), entry.FullPath(), ConflictResolution::AutoRename);
        
        if (!finalPath.empty()) {
            // Set file timestamp
            FileAssembler::SetFileTimestamp(finalPath, entry.lastModified);
            
            // Verify checksum if available
            if (!entry.checksum.empty()) {
                HashAlgorithm algo = Crypto::ParseAlgorithm(entry.checksumType);
                if (!Crypto::VerifyHash(finalPath, entry.checksum, algo)) {
                    LOG_WARN(L"DownloadEngine: checksum mismatch for %s", entry.fileName.c_str());
                }
            }
            
            entry.status = DownloadStatus::Complete;
            entry.dateCompleted = SystemClock::now();
            entry.downloadedBytes = entry.fileSize;
            
            // Clean up state files
            ResumeEngine::CleanupPartialFiles(entry);
            
            NotifyComplete(id);
        } else {
            entry.status = DownloadStatus::Error;
            entry.errorMessage = L"Failed to finalize download";
            NotifyError(id, entry.errorMessage);
        }
        
        m_database.UpdateEntry(entry);
    } else {
        // Incomplete - error or partial completion
        entry.status = DownloadStatus::Error;
        entry.downloadedBytes = segments.GetTotalDownloaded();
        m_database.UpdateEntry(entry);
        ResumeEngine::SaveState(entry, segments);
        
        NotifyError(id, entry.errorMessage.empty() ? L"Download incomplete" : entry.errorMessage);
    }
    
    // Remove from active downloads
    RecursiveLock lock(m_downloadsMutex);
    m_activeDownloads.erase(id);
}

// ─── Connection Worker Thread ──────────────────────────────────────────────
void DownloadEngine::ConnectionWorker(const String& downloadId, int connectionId) {
    std::shared_ptr<ActiveDownload> active;
    {
        RecursiveLock lock(m_downloadsMutex);
        auto it = m_activeDownloads.find(downloadId);
        if (it == m_activeDownloads.end()) return;
        active = it->second;
    }
    
    auto& entry = active->entry;
    auto& segments = active->segments;
    
    int retryCount = 0;
    
    while (!active->cancelled.load() && m_running.load()) {
        // Request a segment to download
        auto splitResult = segments.RequestSegment(connectionId);
        if (!splitResult.success) {
            // No more work available
            LOG_DEBUG(L"Connection %d: no segment available, exiting", connectionId);
            break;
        }
        
        // Wait while paused
        while (active->paused.load() && !active->cancelled.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (active->cancelled.load()) break;
        
        // Create HTTP client for this connection
        auto client = ConnectionPool::Instance().AcquireHttpClient();
        
        HttpRequestConfig config;
        config.url = entry.finalUrl.empty() ? entry.url : entry.finalUrl;
        config.userAgent = entry.userAgent;
        config.referrer = entry.referrer;
        config.cookies = entry.cookies;
        config.username = entry.username;
        config.password = entry.password;
        config.rangeStart = splitResult.newStart;
        config.rangeEnd = splitResult.newEnd;
        
        // Apply proxy
        auto proxy = ProxyManager::Instance().GetProxyForUrl(config.url);
        if (proxy.type != ProxyType::None) {
            config.proxyAddr = proxy.address + L":" + std::to_wstring(proxy.port);
        }
        
        // Speed tracking for this connection
        int64 bytesThisSecond = 0;
        auto secondStart = Clock::now();
        
        HttpResponseInfo response;
        bool success = client->Get(config, response, 
            [&](const uint8* data, size_t length) -> bool {
                if (active->cancelled.load() || active->paused.load()) {
                    return false;
                }
                
                // Apply speed limiter
                size_t offset = 0;
                while (offset < length) {
                    size_t permitted = SpeedLimiter::Instance().RequestBytes(length - offset);
                    if (permitted == 0) permitted = length - offset;
                    
                    // Write to file at the correct position
                    int64 writePos = splitResult.newStart + 
                                     segments.GetSegments()[0].DownloadedBytes();
                    
                    // Find our segment's current position
                    auto segs = segments.GetSegments();
                    for (const auto& s : segs) {
                        if (s.id == splitResult.newSegmentId) {
                            writePos = s.currentPos;
                            break;
                        }
                    }
                    
                    if (!FileAssembler::WriteAtPosition(active->hFile, writePos,
                                                         data + offset, permitted)) {
                        return false;
                    }
                    
                    // Update segment progress
                    segments.UpdateProgress(splitResult.newSegmentId, 
                                           static_cast<int64>(permitted), 0);
                    
                    offset += permitted;
                    bytesThisSecond += static_cast<int64>(permitted);
                }
                
                // Calculate speed every second
                auto now = Clock::now();
                double elapsed = std::chrono::duration<double>(now - secondStart).count();
                if (elapsed >= 1.0) {
                    double speed = bytesThisSecond / elapsed;
                    segments.UpdateProgress(splitResult.newSegmentId, 0, speed);
                    
                    // Notify UI
                    NotifyProgress(active->id, segments.GetTotalDownloaded(),
                                   entry.fileSize, speed);
                    NotifySegmentUpdate(active->id, segments.GetSegments());
                    
                    // Update database progress
                    m_database.UpdateProgress(active->id, segments.GetTotalDownloaded(),
                                             speed, segments.ToSegmentInfoVector());
                    
                    bytesThisSecond = 0;
                    secondStart = now;
                }
                
                return true;
            });
        
        ConnectionPool::Instance().ReleaseHttpClient(std::move(client));
        
        if (success || active->cancelled.load()) {
            if (success) {
                segments.MarkComplete(splitResult.newSegmentId);
            }
            retryCount = 0;
        } else {
            // Error handling with retry
            segments.MarkError(splitResult.newSegmentId);
            retryCount++;
            
            if (retryCount >= entry.maxRetries) {
                LOG_ERROR(L"Connection %d: max retries (%d) exhausted", 
                          connectionId, entry.maxRetries);
                break;
            }
            
            int delay = ResumeEngine::GetRetryDelay(retryCount);
            LOG_WARN(L"Connection %d: retry %d/%d in %ds", 
                     connectionId, retryCount, entry.maxRetries, delay);
            
            // Wait for retry delay (interruptible)
            for (int i = 0; i < delay * 10 && !active->cancelled.load(); ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }
}

// ─── Pause / Stop / Remove ─────────────────────────────────────────────────
bool DownloadEngine::PauseDownload(const String& id) {
    RecursiveLock lock(m_downloadsMutex);
    
    auto it = m_activeDownloads.find(id);
    if (it != m_activeDownloads.end()) {
        it->second->paused.store(true);
        it->second->cancelled.store(true);  // Signal threads to exit
        NotifyPaused(id);
        return true;
    }
    
    // Mark as paused in database even if not active
    auto entry = m_database.GetEntry(id);
    if (entry.has_value()) {
        entry->status = DownloadStatus::Paused;
        m_database.UpdateEntry(*entry);
        NotifyPaused(id);
        return true;
    }
    
    return false;
}

bool DownloadEngine::StopDownload(const String& id) {
    RecursiveLock lock(m_downloadsMutex);
    
    auto it = m_activeDownloads.find(id);
    if (it != m_activeDownloads.end()) {
        it->second->cancelled.store(true);
        return true;
    }
    return false;
}

void DownloadEngine::StopAll() {
    RecursiveLock lock(m_downloadsMutex);
    
    for (auto& [id, active] : m_activeDownloads) {
        active->cancelled.store(true);
    }
    
    // Wait for all downloads to stop
    while (!m_activeDownloads.empty()) {
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        lock.lock();
    }
}

bool DownloadEngine::RemoveDownload(const String& id, bool deleteFiles) {
    StopDownload(id);
    
    // Wait a moment for download thread to exit
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    m_database.RemoveEntry(id, deleteFiles);
    NotifyRemoved(id);
    return true;
}

int DownloadEngine::RemoveCompleted() {
    return m_database.RemoveCompleted(false);
}

void DownloadEngine::ResumeAll() {
    auto entries = m_database.GetEntriesByStatus(DownloadStatus::Paused);
    for (const auto& entry : entries) {
        StartDownload(entry.id);
    }
}

// ─── Query Interface ───────────────────────────────────────────────────────
std::vector<DownloadEntry> DownloadEngine::GetAllDownloads() const {
    return m_database.GetAllEntries();
}

std::optional<DownloadEntry> DownloadEngine::GetDownload(const String& id) const {
    // Check active downloads first (fresher data)
    RecursiveLock lock(m_downloadsMutex);
    auto it = m_activeDownloads.find(id);
    if (it != m_activeDownloads.end()) {
        return it->second->entry;
    }
    return m_database.GetEntry(id);
}

int DownloadEngine::GetActiveCount() const {
    RecursiveLock lock(m_downloadsMutex);
    return static_cast<int>(m_activeDownloads.size());
}

double DownloadEngine::GetTotalSpeed() const {
    RecursiveLock lock(m_downloadsMutex);
    double total = 0;
    for (const auto& [id, active] : m_activeDownloads) {
        total += active->totalSpeed.load();
    }
    return total;
}

// ─── Probe URL ─────────────────────────────────────────────────────────────
bool DownloadEngine::ProbeUrl(const String& url, HttpResponseInfo& response,
                               String& suggestedFileName, String& category) {
    auto client = ConnectionPool::Instance().AcquireHttpClient();
    
    HttpRequestConfig config;
    config.url = url;
    config.userAgent = constants::DEFAULT_USER_AGENT;
    
    bool ok = client->Head(config, response);
    ConnectionPool::Instance().ReleaseHttpClient(std::move(client));
    
    if (ok && response.statusCode < 400) {
        // Determine filename
        suggestedFileName = response.GetDispositionFilename();
        if (suggestedFileName.empty()) {
            suggestedFileName = Unicode::ExtractFilenameFromUrl(
                response.finalUrl.empty() ? url : response.finalUrl);
        }
        
        // Determine category
        String ext = Unicode::GetFileExtension(suggestedFileName);
        category = Unicode::CategorizeByExtension(ext);
        
        return true;
    }
    
    return false;
}

// ─── Observer Management ───────────────────────────────────────────────────
void DownloadEngine::AddObserver(IDownloadObserver* observer) {
    Lock lock(m_observerMutex);
    m_observers.push_back(observer);
}

void DownloadEngine::RemoveObserver(IDownloadObserver* observer) {
    Lock lock(m_observerMutex);
    m_observers.erase(
        std::remove(m_observers.begin(), m_observers.end(), observer),
        m_observers.end());
}

// Notification helpers
#define NOTIFY_OBSERVERS(method, ...) \
    { Lock lock(m_observerMutex); \
      for (auto* obs : m_observers) { obs->method(__VA_ARGS__); } }

void DownloadEngine::NotifyAdded(const String& id)   { NOTIFY_OBSERVERS(OnDownloadAdded, id) }
void DownloadEngine::NotifyStarted(const String& id)  { NOTIFY_OBSERVERS(OnDownloadStarted, id) }
void DownloadEngine::NotifyProgress(const String& id, int64 d, int64 t, double s) 
    { NOTIFY_OBSERVERS(OnDownloadProgress, id, d, t, s) }
void DownloadEngine::NotifySegmentUpdate(const String& id, const std::vector<Segment>& segs)
    { NOTIFY_OBSERVERS(OnDownloadSegmentUpdate, id, segs) }
void DownloadEngine::NotifyComplete(const String& id) { NOTIFY_OBSERVERS(OnDownloadComplete, id) }
void DownloadEngine::NotifyError(const String& id, const String& e) 
    { NOTIFY_OBSERVERS(OnDownloadError, id, e) }
void DownloadEngine::NotifyPaused(const String& id)   { NOTIFY_OBSERVERS(OnDownloadPaused, id) }
void DownloadEngine::NotifyResumed(const String& id)  { NOTIFY_OBSERVERS(OnDownloadResumed, id) }
void DownloadEngine::NotifyRemoved(const String& id)  { NOTIFY_OBSERVERS(OnDownloadRemoved, id) }
void DownloadEngine::NotifySpeed(double s, int c)     { NOTIFY_OBSERVERS(OnSpeedUpdate, s, c) }

#undef NOTIFY_OBSERVERS

// ─── Background Threads ────────────────────────────────────────────────────
void DownloadEngine::SpeedMonitorThread() {
    while (m_running.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        double totalSpeed = 0;
        int activeCount = 0;
        
        {
            RecursiveLock lock(m_downloadsMutex);
            for (auto& [id, active] : m_activeDownloads) {
                auto segs = active->segments.GetSegments();
                double dlSpeed = 0;
                for (const auto& s : segs) {
                    if (s.status == SegmentStatus::Active) dlSpeed += s.speed;
                }
                active->totalSpeed.store(dlSpeed);
                totalSpeed += dlSpeed;
                if (!active->cancelled.load()) activeCount++;
            }
        }
        
        SpeedLimiter::Instance().UpdateCurrentTotalSpeed(totalSpeed);
        NotifySpeed(totalSpeed, activeCount);
    }
}

void DownloadEngine::StatePersistThread() {
    while (m_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(constants::SEGMENT_SAVE_INTERVAL_MS));
        
        RecursiveLock lock(m_downloadsMutex);
        for (auto& [id, active] : m_activeDownloads) {
            if (!active->cancelled.load()) {
                ResumeEngine::SaveState(active->entry, active->segments);
            }
        }
        
        // Periodic database flush
        m_database.Flush();
    }
}

} // namespace idm
