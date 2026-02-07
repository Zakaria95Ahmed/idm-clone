/**
 * @file Database.h
 * @brief Lightweight embedded database for download persistence
 *
 * Architecture decision: Rather than depending on SQLite (which would add
 * a ~1MB dependency), we implement a custom binary database format that's
 * optimized for IDM's specific access patterns:
 *
 *   1. Sequential scans (loading all downloads at startup)
 *   2. Single-record updates (progress changes during download)
 *   3. Append-only inserts (new downloads)
 *   4. Occasional deletes (user removes completed downloads)
 *
 * The database file format:
 *   [Header: 64 bytes]
 *   [Record 1: variable length]
 *   [Record 2: variable length]
 *   ...
 *
 * Each record is self-describing with a length prefix and type tag,
 * enabling forward compatibility when fields are added.
 *
 * For crash safety, we use a write-ahead approach: changes are first
 * written to a .journal file, then applied to the main database,
 * then the journal is deleted. On startup, if a journal exists,
 * it's replayed to recover from a crash during write.
 */

#pragma once
#include "stdafx.h"

namespace idm {

// ─── Download Status Enumeration ───────────────────────────────────────────
enum class DownloadStatus : int {
    Queued       = 0,
    Connecting   = 1,
    Downloading  = 2,
    Paused       = 3,
    Complete     = 4,
    Error        = 5,
    Waiting      = 6,   // Waiting for retry
    Merging      = 7    // Assembling segments
};

// ─── Segment State ─────────────────────────────────────────────────────────
struct SegmentInfo {
    int64   startByte;      // Start position in the file
    int64   endByte;        // End position (inclusive)
    int64   downloadedBytes;// Bytes downloaded in this segment
    int     connectionId;   // Which connection owns this segment (-1 = unassigned)
    bool    complete;       // Segment fully downloaded
    
    int64 RemainingBytes() const {
        return (endByte - startByte + 1) - downloadedBytes;
    }
    
    int64 CurrentPosition() const {
        return startByte + downloadedBytes;
    }
    
    int64 TotalBytes() const {
        return endByte - startByte + 1;
    }
    
    double ProgressPercent() const {
        int64 total = TotalBytes();
        return total > 0 ? (static_cast<double>(downloadedBytes) / total * 100.0) : 0.0;
    }
};

// ─── Download Entry ────────────────────────────────────────────────────────
// Complete representation of a single download item, matching IDM's data model
struct DownloadEntry {
    // Identity
    String              id;             // GUID string
    String              url;            // Original URL
    String              finalUrl;       // URL after redirects
    String              fileName;       // Target file name
    String              savePath;       // Full save directory path
    
    // Size and progress
    int64               fileSize;       // Total file size (-1 if unknown)
    int64               downloadedBytes;// Total bytes downloaded
    DownloadStatus      status;
    
    // Classification
    String              category;       // Music, Video, Programs, etc.
    String              description;    // User description
    
    // Timestamps
    SystemTimePoint     dateAdded;
    SystemTimePoint     dateCompleted;
    
    // Request metadata
    String              referrer;
    String              cookies;
    String              userAgent;
    String              username;       // HTTP auth
    String              password;
    String              postData;
    
    // Connection configuration
    int                 numConnections; // Requested connection count
    std::vector<SegmentInfo> segments;  // Current segment map
    
    // Resume support
    bool                resumeSupported;
    String              etag;           // Server ETag for validation
    String              lastModified;   // Server Last-Modified
    String              contentType;
    
    // Error handling
    String              errorMessage;
    int                 retryCount;
    int                 maxRetries;
    
    // Queue association
    String              queueId;
    int                 queuePosition;
    
    // Integrity
    String              checksum;       // Expected hash (if known)
    String              checksumType;   // MD5, SHA1, SHA256
    
    // Speed tracking
    double              currentSpeed;   // bytes/sec
    double              averageSpeed;
    std::deque<double>  speedHistory;   // Last 60 seconds of speed samples
    
    // Constructor with sensible defaults
    DownloadEntry() 
        : fileSize(-1)
        , downloadedBytes(0)
        , status(DownloadStatus::Queued)
        , numConnections(constants::DEFAULT_MAX_CONNECTIONS)
        , resumeSupported(false)
        , retryCount(0)
        , maxRetries(constants::DEFAULT_RETRY_COUNT)
        , queuePosition(-1)
        , currentSpeed(0.0)
        , averageSpeed(0.0) 
    {
        dateAdded = SystemClock::now();
    }
    
    // Generate a unique ID for new downloads
    static String GenerateId();
    
    // Compute overall progress percentage
    double ProgressPercent() const {
        if (fileSize <= 0) return 0.0;
        return static_cast<double>(downloadedBytes) / fileSize * 100.0;
    }
    
    // Get human-readable status string
    String StatusString() const;
    
    // Get full file path
    String FullPath() const {
        return savePath + L"\\" + fileName;
    }
    
    // Get partial file path (during download)
    String PartialPath() const {
        return FullPath() + L".idmclone";
    }
    
    // Get segment state file path
    String SegmentPath() const {
        return FullPath() + L".seg";
    }
};

// ─── Database Class ────────────────────────────────────────────────────────
class Database {
public:
    Database();
    ~Database();
    
    /**
     * Open or create the download database at the specified path.
     * If the database exists, all entries are loaded into memory.
     * If a journal file exists, it's replayed for crash recovery.
     */
    bool Open(const String& dbPath);
    
    /**
     * Close the database, flushing all pending changes.
     */
    void Close();
    
    /**
     * Add a new download entry. Assigns an ID if not already set.
     * @return The entry's ID
     */
    String AddEntry(DownloadEntry& entry);
    
    /**
     * Update an existing entry (identified by ID).
     * Only the modified fields need to be set; others are preserved.
     */
    bool UpdateEntry(const DownloadEntry& entry);
    
    /**
     * Update only the progress fields (bytes downloaded, speed, segments).
     * This is a fast path used during active downloads to minimize I/O.
     */
    bool UpdateProgress(const String& id, int64 downloadedBytes,
                        double speed, const std::vector<SegmentInfo>& segments);
    
    /**
     * Remove an entry by ID.
     * @param deleteFiles  If true, also delete the downloaded file and partials
     */
    bool RemoveEntry(const String& id, bool deleteFiles = false);
    
    /**
     * Get a single entry by ID.
     */
    std::optional<DownloadEntry> GetEntry(const String& id) const;
    
    /**
     * Get all entries, optionally filtered by status or category.
     */
    std::vector<DownloadEntry> GetAllEntries() const;
    std::vector<DownloadEntry> GetEntriesByStatus(DownloadStatus status) const;
    std::vector<DownloadEntry> GetEntriesByCategory(const String& category) const;
    
    /**
     * Get count of entries by status.
     */
    int GetCountByStatus(DownloadStatus status) const;
    int GetTotalCount() const;
    
    /**
     * Flush all in-memory changes to disk.
     */
    bool Flush();
    
    /**
     * Remove all completed downloads from the database.
     */
    int RemoveCompleted(bool deleteFiles = false);
    
private:
    bool LoadFromDisk();
    bool SaveToDisk();
    bool WriteJournal(const DownloadEntry& entry, const String& operation);
    bool ReplayJournal();
    void CleanJournal();
    
    // Serialization helpers
    std::vector<uint8> SerializeEntry(const DownloadEntry& entry) const;
    DownloadEntry DeserializeEntry(const uint8* data, size_t length) const;
    
    String                              m_dbPath;
    String                              m_journalPath;
    std::map<String, DownloadEntry>     m_entries;     // ID -> Entry
    mutable RecursiveMutex              m_mutex;
    bool                                m_dirty{false};
};

} // namespace idm
