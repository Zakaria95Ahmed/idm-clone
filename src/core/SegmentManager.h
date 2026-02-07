/**
 * @file SegmentManager.h
 * @brief Dynamic file segmentation engine - IDM's signature technology
 *
 * This is the core innovation that makes IDM fast. The algorithm:
 *
 * 1. Download starts with a SINGLE connection for the whole file
 * 2. When a new connection slot becomes available:
 *    a. Find the segment with the LARGEST remaining bytes
 *    b. Split that segment in half at the midpoint
 *    c. The new connection starts downloading from the midpoint
 * 3. When a connection finishes its segment:
 *    a. If the next segment hasn't started, take it over
 *    b. If all adjacent segments are active, find the largest
 *       remaining segment anywhere and split it
 * 4. This keeps ALL connections busy at ALL times
 * 5. Minimum segment size prevents excessive splitting
 *
 * Segment state is persisted to disk every 15 seconds for crash recovery.
 * The state file (.seg) contains a binary map of all segment boundaries
 * and their download progress.
 *
 * Thread safety: All public methods are synchronized. The segment map
 * is the shared state accessed by multiple download threads.
 */

#pragma once
#include "stdafx.h"
#include "../util/Database.h"

namespace idm {

// ─── Segment Status ────────────────────────────────────────────────────────
enum class SegmentStatus {
    Pending,        // Not yet started
    Active,         // Currently being downloaded
    Complete,       // Fully downloaded
    Error           // Failed (will be retried)
};

// ─── Extended Segment Info ─────────────────────────────────────────────────
struct Segment {
    int                 id;             // Segment index
    int64               startByte;      // Start position in file
    int64               endByte;        // End position (inclusive)
    int64               currentPos;     // Current download position
    int                 connectionId;   // Owning connection (-1 = unassigned)
    SegmentStatus       status;
    TimePoint           lastActivity;   // For detecting stalled connections
    double              speed;          // Current speed for this segment
    
    int64 TotalBytes() const { return endByte - startByte + 1; }
    int64 DownloadedBytes() const { return currentPos - startByte; }
    int64 RemainingBytes() const { return endByte - currentPos + 1; }
    double Progress() const {
        int64 total = TotalBytes();
        return total > 0 ? static_cast<double>(DownloadedBytes()) / total * 100.0 : 0.0;
    }
    bool IsComplete() const { return currentPos > endByte; }
};

// ─── Segment Split Result ──────────────────────────────────────────────────
struct SplitResult {
    bool        success;
    int         newSegmentId;
    int64       newStart;       // Start byte for the new segment
    int64       newEnd;         // End byte for the new segment
    int         parentSegmentId;// Which segment was split
};

class SegmentManager {
public:
    /**
     * Initialize for a new download.
     * @param fileSize      Total file size (-1 if unknown)
     * @param maxConnections Maximum number of simultaneous connections
     * @param minSegmentSize Minimum segment size (prevent over-splitting)
     */
    void Initialize(int64 fileSize, int maxConnections, 
                    int64 minSegmentSize = constants::MIN_SEGMENT_SIZE);
    
    /**
     * Load segment state from a previous session (resume).
     * @param segments Previously saved segment information
     * @return true if the state was loaded successfully
     */
    bool LoadState(const std::vector<SegmentInfo>& segments);
    
    /**
     * Request a segment for a new connection.
     * This implements IDM's dynamic splitting algorithm:
     * - If there are unassigned segments, return the first one
     * - Otherwise, find the largest active segment and split it
     * 
     * @param connectionId ID of the connection requesting work
     * @return SplitResult with the segment assignment
     */
    SplitResult RequestSegment(int connectionId);
    
    /**
     * Update the download progress for a segment.
     * Called by download threads as data is received.
     * 
     * @param segmentId    The segment being updated
     * @param bytesWritten Number of new bytes downloaded
     * @param speed        Current download speed for this connection
     */
    void UpdateProgress(int segmentId, int64 bytesWritten, double speed);
    
    /**
     * Mark a segment as complete.
     * This triggers redistribution: if the completing connection can
     * take over adjacent unstarted segments or split the largest
     * remaining segment.
     */
    void MarkComplete(int segmentId);
    
    /**
     * Mark a segment as failed. It will be retried.
     */
    void MarkError(int segmentId);
    
    /**
     * Release a segment back to unassigned (connection dropped).
     */
    void ReleaseSegment(int segmentId);
    
    /**
     * Get current segment map for UI display and persistence.
     */
    std::vector<Segment> GetSegments() const;
    
    /**
     * Convert to SegmentInfo vector for database storage.
     */
    std::vector<SegmentInfo> ToSegmentInfoVector() const;
    
    /**
     * Get overall progress.
     */
    int64 GetTotalDownloaded() const;
    int64 GetFileSize() const;
    double GetOverallProgress() const;
    
    /**
     * Check if all segments are complete.
     */
    bool IsComplete() const;
    
    /**
     * Get the number of active connections.
     */
    int GetActiveConnectionCount() const;
    
    /**
     * Get the number of segments.
     */
    int GetSegmentCount() const;
    
    /**
     * Set maximum connections (can be changed at runtime).
     */
    void SetMaxConnections(int maxConn);
    int GetMaxConnections() const;
    
    /**
     * Save segment state to disk for crash recovery.
     */
    bool SaveState(const String& filePath) const;
    bool LoadStateFromFile(const String& filePath);
    
private:
    // Find the segment with the largest remaining bytes
    int FindLargestActiveSegment() const;
    
    // Find the first unassigned pending segment
    int FindPendingSegment() const;
    
    // Split a segment at its midpoint, returns the new segment ID
    int SplitSegment(int segmentId);
    
    mutable RecursiveMutex      m_mutex;
    std::vector<Segment>        m_segments;
    int64                       m_fileSize{-1};
    int                         m_maxConnections{8};
    int64                       m_minSegmentSize{constants::MIN_SEGMENT_SIZE};
    int                         m_nextSegmentId{0};
};

} // namespace idm
