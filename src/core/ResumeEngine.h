/**
 * @file ResumeEngine.h
 * @brief Download resume and error recovery engine
 *
 * Handles: pause/resume, crash recovery, server validation on resume,
 * automatic retry with configurable attempts and delays.
 */

#pragma once
#include "stdafx.h"
#include "../util/Database.h"

namespace idm {

class SegmentManager;
class HttpClient;

class ResumeEngine {
public:
    /**
     * Check if a download can be resumed by validating with the server.
     * Compares ETag and Last-Modified headers to detect file changes.
     */
    static bool ValidateResume(HttpClient& client, DownloadEntry& entry);
    
    /**
     * Save download state for crash recovery.
     * Writes segment positions to the .seg file and updates the database.
     */
    static bool SaveState(const DownloadEntry& entry, const SegmentManager& segments);
    
    /**
     * Restore download state from disk.
     * Loads the .seg file and validates segment boundaries.
     */
    static bool RestoreState(DownloadEntry& entry, SegmentManager& segments);
    
    /**
     * Clean up partial/state files after successful completion.
     */
    static void CleanupPartialFiles(const DownloadEntry& entry);
    
    /**
     * Check if partial files exist for a download.
     */
    static bool HasPartialFiles(const DownloadEntry& entry);
    
    /**
     * Determine retry delay using exponential backoff.
     * Base delay is multiplied by 2^(retryCount-1), capped at 5 minutes.
     */
    static int GetRetryDelay(int retryCount, int baseDelaySec = 5);
    
    /**
     * Check if we should retry based on the error type.
     * Network errors: yes. Server 404: no. Server 503: yes.
     */
    static bool ShouldRetry(DWORD errorCode, int httpStatusCode);
};

} // namespace idm
