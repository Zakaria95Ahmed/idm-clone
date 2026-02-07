/**
 * @file DownloadEngine.h
 * @brief Main download engine orchestrating all core components
 *
 * Architecture: Singleton engine managing all downloads.
 * Uses a thread pool for concurrent downloads, with each download
 * further using multiple threads for segmented transfer.
 *
 * The engine follows the Observer pattern: UI components register
 * as observers and receive notifications about download state changes,
 * progress updates, completions, and errors.
 */

#pragma once
#include "stdafx.h"
#include "../util/Database.h"
#include "SegmentManager.h"
#include "HttpClient.h"

namespace idm {

// ─── Observer Interface ────────────────────────────────────────────────────
class IDownloadObserver {
public:
    virtual ~IDownloadObserver() = default;
    
    virtual void OnDownloadAdded(const String& /*id*/) {}
    virtual void OnDownloadStarted(const String& /*id*/) {}
    virtual void OnDownloadProgress(const String& /*id*/, int64 /*downloaded*/, 
                                     int64 /*total*/, double /*speed*/) {}
    virtual void OnDownloadSegmentUpdate(const String& /*id*/, 
                                          const std::vector<Segment>& /*segments*/) {}
    virtual void OnDownloadComplete(const String& /*id*/) {}
    virtual void OnDownloadError(const String& /*id*/, const String& /*error*/) {}
    virtual void OnDownloadPaused(const String& /*id*/) {}
    virtual void OnDownloadResumed(const String& /*id*/) {}
    virtual void OnDownloadRemoved(const String& /*id*/) {}
    virtual void OnSpeedUpdate(double /*totalSpeed*/, int /*activeCount*/) {}
};

// ─── Active Download State ─────────────────────────────────────────────────
struct ActiveDownload {
    String                          id;
    DownloadEntry                   entry;
    SegmentManager                  segments;
    HANDLE                          hFile{INVALID_HANDLE_VALUE};
    std::vector<std::thread>        connectionThreads;
    std::atomic<bool>               cancelled{false};
    std::atomic<bool>               paused{false};
    std::atomic<double>             totalSpeed{0};
    TimePoint                       startTime;
    TimePoint                       lastStateSave;
};

// ─── Download Engine ───────────────────────────────────────────────────────
class DownloadEngine {
public:
    static DownloadEngine& Instance();
    
    DownloadEngine(const DownloadEngine&) = delete;
    DownloadEngine& operator=(const DownloadEngine&) = delete;
    
    /**
     * Initialize the engine. Must be called at startup.
     */
    bool Initialize(const String& dataDir);
    
    /**
     * Shutdown the engine. Stops all downloads and saves state.
     */
    void Shutdown();
    
    // ─── Download Operations ───────────────────────────────────────────────
    
    /**
     * Add a new download from URL. Returns the download ID.
     * If startImmediately is true, the download begins right away.
     */
    String AddDownload(const String& url, const String& savePath,
                       const String& fileName = L"",
                       const String& referrer = L"",
                       const String& cookies = L"",
                       bool startImmediately = true);
    
    /**
     * Add a download from a pre-configured entry.
     */
    String AddDownload(DownloadEntry& entry, bool startImmediately = true);
    
    /**
     * Start/resume a paused or queued download.
     */
    bool StartDownload(const String& id);
    
    /**
     * Pause an active download.
     */
    bool PauseDownload(const String& id);
    
    /**
     * Stop (cancel) an active download.
     */
    bool StopDownload(const String& id);
    
    /**
     * Stop all active downloads.
     */
    void StopAll();
    
    /**
     * Remove a download entry.
     * @param deleteFiles Also delete downloaded/partial files
     */
    bool RemoveDownload(const String& id, bool deleteFiles = false);
    
    /**
     * Remove all completed downloads.
     */
    int RemoveCompleted();
    
    /**
     * Resume all paused downloads.
     */
    void ResumeAll();
    
    // ─── Query Interface ───────────────────────────────────────────────────
    
    std::vector<DownloadEntry> GetAllDownloads() const;
    std::optional<DownloadEntry> GetDownload(const String& id) const;
    int GetActiveCount() const;
    double GetTotalSpeed() const;
    
    // ─── Observer Management ───────────────────────────────────────────────
    
    void AddObserver(IDownloadObserver* observer);
    void RemoveObserver(IDownloadObserver* observer);
    
    // ─── Probe URL ─────────────────────────────────────────────────────────
    
    /**
     * Send HEAD request to get file info without downloading.
     * Used by the Add URL dialog to populate file details.
     */
    bool ProbeUrl(const String& url, HttpResponseInfo& response,
                  String& suggestedFileName, String& category);
    
    // ─── Database Access ───────────────────────────────────────────────────
    Database& GetDatabase() { return m_database; }
    
private:
    DownloadEngine() = default;
    ~DownloadEngine();
    
    // Download worker thread - manages all connections for one download
    void DownloadWorker(const String& id);
    
    // Connection worker thread - downloads a single segment
    void ConnectionWorker(const String& downloadId, int connectionId);
    
    // Speed monitoring thread
    void SpeedMonitorThread();
    
    // State persistence thread (saves segment state periodically)
    void StatePersistThread();
    
    // Notify all observers
    void NotifyAdded(const String& id);
    void NotifyStarted(const String& id);
    void NotifyProgress(const String& id, int64 downloaded, int64 total, double speed);
    void NotifySegmentUpdate(const String& id, const std::vector<Segment>& segments);
    void NotifyComplete(const String& id);
    void NotifyError(const String& id, const String& error);
    void NotifyPaused(const String& id);
    void NotifyResumed(const String& id);
    void NotifyRemoved(const String& id);
    void NotifySpeed(double totalSpeed, int activeCount);
    
    // State
    Database                                    m_database;
    String                                      m_dataDir;
    bool                                        m_initialized{false};
    std::atomic<bool>                           m_running{false};
    
    // Active downloads
    mutable RecursiveMutex                      m_downloadsMutex;
    std::map<String, std::shared_ptr<ActiveDownload>> m_activeDownloads;
    
    // Observers
    mutable Mutex                               m_observerMutex;
    std::vector<IDownloadObserver*>             m_observers;
    
    // Background threads
    std::thread                                 m_speedMonitor;
    std::thread                                 m_statePersist;
};

} // namespace idm
