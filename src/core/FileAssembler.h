/**
 * @file FileAssembler.h
 * @brief Segment merge and final file assembly engine
 *
 * After all segments complete downloading to the partial file (.idmclone),
 * the FileAssembler renames it to the target filename. Since segments
 * are written directly to their correct positions in the partial file
 * using random-access I/O, no actual merging/copying is needed.
 *
 * Post-assembly operations:
 *   - Verify file integrity (checksum if available)
 *   - Preserve original file modification timestamp
 *   - Handle naming conflicts (auto-rename, overwrite, skip)
 */

#pragma once
#include "stdafx.h"
#include "../util/Database.h"

namespace idm {

enum class ConflictResolution {
    AutoRename,     // file.txt -> file(1).txt
    Overwrite,      // Replace existing
    Skip            // Cancel if exists
};

class FileAssembler {
public:
    /**
     * Open the partial file for writing. Creates or opens the .idmclone file.
     * The file is pre-allocated to the full size for better disk performance
     * and to ensure space is available.
     */
    static HANDLE OpenPartialFile(const String& partialPath, int64 fileSize);
    
    /**
     * Write data to a specific position in the partial file.
     * Used by download threads to write their segment data.
     * Thread-safe: uses file-level locking for the write region.
     */
    static bool WriteAtPosition(HANDLE hFile, int64 position, 
                                const uint8* data, size_t length);
    
    /**
     * Finalize the download: rename partial to target, verify, clean up.
     * @param partialPath   Path to the .idmclone partial file
     * @param targetPath    Final file path
     * @param conflictMode  How to handle existing files
     * @return The actual final path (may differ if auto-renamed)
     */
    static String Finalize(const String& partialPath, const String& targetPath,
                           ConflictResolution conflictMode = ConflictResolution::AutoRename);
    
    /**
     * Generate an auto-renamed path: file.txt -> file(1).txt -> file(2).txt
     */
    static String GenerateUniqueName(const String& targetPath);
    
    /**
     * Set file modification time from server Last-Modified header.
     */
    static bool SetFileTimestamp(const String& filePath, const String& lastModified);
    
    /**
     * Pre-allocate file to the specified size (improves write performance).
     */
    static bool PreallocateFile(HANDLE hFile, int64 fileSize);
};

} // namespace idm
