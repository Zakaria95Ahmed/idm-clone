/**
 * @file SegmentManager.cpp
 * @brief Dynamic segmentation engine implementation
 *
 * This implements IDM's core "dynamic file segmentation" technology.
 * The key insight: rather than pre-splitting the file into N equal parts
 * (which wastes connections when some parts finish early), we dynamically
 * split segments as connections become available. This ensures ALL
 * connections are always downloading useful data.
 *
 * The algorithm keeps connections maximally utilized by always finding
 * the largest remaining chunk of work and splitting it for a new worker.
 */

#include "stdafx.h"
#include "SegmentManager.h"
#include "../util/Logger.h"

namespace idm {

// ─── Initialize ────────────────────────────────────────────────────────────
void SegmentManager::Initialize(int64 fileSize, int maxConnections, int64 minSegmentSize) {
    RecursiveLock lock(m_mutex);
    
    m_fileSize = fileSize;
    m_maxConnections = maxConnections;
    m_minSegmentSize = minSegmentSize;
    m_nextSegmentId = 0;
    m_segments.clear();
    
    if (fileSize <= 0) {
        // Unknown file size: single connection, no segmentation
        Segment seg;
        seg.id = m_nextSegmentId++;
        seg.startByte = 0;
        seg.endByte = INT64_MAX;  // Will be adjusted when we know the size
        seg.currentPos = 0;
        seg.connectionId = -1;
        seg.status = SegmentStatus::Pending;
        seg.speed = 0;
        m_segments.push_back(seg);
        
        LOG_INFO(L"SegmentManager: initialized with unknown file size (single connection)");
    } else {
        // Known file size: start with a single segment covering the whole file
        // Additional segments will be created dynamically as connections request work
        Segment seg;
        seg.id = m_nextSegmentId++;
        seg.startByte = 0;
        seg.endByte = fileSize - 1;
        seg.currentPos = 0;
        seg.connectionId = -1;
        seg.status = SegmentStatus::Pending;
        seg.speed = 0;
        m_segments.push_back(seg);
        
        LOG_INFO(L"SegmentManager: initialized for %lld bytes, max %d connections",
                 fileSize, maxConnections);
    }
}

// ─── Load State (Resume) ──────────────────────────────────────────────────
bool SegmentManager::LoadState(const std::vector<SegmentInfo>& segments) {
    RecursiveLock lock(m_mutex);
    
    m_segments.clear();
    m_nextSegmentId = 0;
    
    for (const auto& info : segments) {
        Segment seg;
        seg.id = m_nextSegmentId++;
        seg.startByte = info.startByte;
        seg.endByte = info.endByte;
        seg.currentPos = info.startByte + info.downloadedBytes;
        seg.connectionId = -1;  // All connections reset on resume
        seg.status = info.complete ? SegmentStatus::Complete : SegmentStatus::Pending;
        seg.speed = 0;
        m_segments.push_back(seg);
    }
    
    LOG_INFO(L"SegmentManager: loaded %zu segments from previous session", segments.size());
    return !segments.empty();
}

// ─── Request Segment (Dynamic Splitting Algorithm) ─────────────────────────
SplitResult SegmentManager::RequestSegment(int connectionId) {
    RecursiveLock lock(m_mutex);
    
    SplitResult result = {};
    result.success = false;
    
    // Count active connections
    int activeCount = GetActiveConnectionCount();
    if (activeCount >= m_maxConnections) {
        LOG_DEBUG(L"SegmentManager: max connections reached (%d/%d)", 
                  activeCount, m_maxConnections);
        return result;
    }
    
    // Strategy 1: Find an unassigned pending segment
    int pendingIdx = FindPendingSegment();
    if (pendingIdx >= 0) {
        auto& seg = m_segments[pendingIdx];
        seg.connectionId = connectionId;
        seg.status = SegmentStatus::Active;
        seg.lastActivity = Clock::now();
        
        result.success = true;
        result.newSegmentId = seg.id;
        result.newStart = seg.currentPos;
        result.newEnd = seg.endByte;
        result.parentSegmentId = -1;
        
        LOG_DEBUG(L"SegmentManager: assigned pending segment %d to connection %d "
                  L"(bytes %lld-%lld)", seg.id, connectionId, result.newStart, result.newEnd);
        return result;
    }
    
    // Strategy 2: Dynamic splitting - find the largest active segment and split it
    int largestIdx = FindLargestActiveSegment();
    if (largestIdx >= 0) {
        auto& parentSeg = m_segments[largestIdx];
        int64 remaining = parentSeg.RemainingBytes();
        
        // Only split if the remaining bytes are large enough for two viable segments
        if (remaining >= m_minSegmentSize * 2) {
            int newSegId = SplitSegment(largestIdx);
            if (newSegId >= 0) {
                // Find the new segment and assign it
                for (auto& seg : m_segments) {
                    if (seg.id == newSegId) {
                        seg.connectionId = connectionId;
                        seg.status = SegmentStatus::Active;
                        seg.lastActivity = Clock::now();
                        
                        result.success = true;
                        result.newSegmentId = seg.id;
                        result.newStart = seg.startByte;
                        result.newEnd = seg.endByte;
                        result.parentSegmentId = parentSeg.id;
                        
                        LOG_INFO(L"SegmentManager: split segment %d, new segment %d "
                                 L"for connection %d (bytes %lld-%lld)",
                                 parentSeg.id, newSegId, connectionId, 
                                 result.newStart, result.newEnd);
                        return result;
                    }
                }
            }
        }
    }
    
    LOG_DEBUG(L"SegmentManager: no work available for connection %d", connectionId);
    return result;
}

// ─── Update Progress ───────────────────────────────────────────────────────
void SegmentManager::UpdateProgress(int segmentId, int64 bytesWritten, double speed) {
    RecursiveLock lock(m_mutex);
    
    for (auto& seg : m_segments) {
        if (seg.id == segmentId) {
            seg.currentPos += bytesWritten;
            seg.speed = speed;
            seg.lastActivity = Clock::now();
            
            // Check if segment is now complete
            if (seg.currentPos > seg.endByte) {
                seg.currentPos = seg.endByte + 1;
                seg.status = SegmentStatus::Complete;
                LOG_DEBUG(L"SegmentManager: segment %d auto-completed via progress update", segmentId);
            }
            return;
        }
    }
}

// ─── Mark Segment Complete ─────────────────────────────────────────────────
void SegmentManager::MarkComplete(int segmentId) {
    RecursiveLock lock(m_mutex);
    
    for (auto& seg : m_segments) {
        if (seg.id == segmentId) {
            seg.status = SegmentStatus::Complete;
            seg.currentPos = seg.endByte + 1;
            seg.speed = 0;
            LOG_INFO(L"SegmentManager: segment %d completed (bytes %lld-%lld)",
                     segmentId, seg.startByte, seg.endByte);
            return;
        }
    }
}

// ─── Mark Segment Error ────────────────────────────────────────────────────
void SegmentManager::MarkError(int segmentId) {
    RecursiveLock lock(m_mutex);
    
    for (auto& seg : m_segments) {
        if (seg.id == segmentId) {
            seg.status = SegmentStatus::Error;
            seg.connectionId = -1;
            seg.speed = 0;
            LOG_WARN(L"SegmentManager: segment %d errored at position %lld",
                     segmentId, seg.currentPos);
            return;
        }
    }
}

// ─── Release Segment ───────────────────────────────────────────────────────
void SegmentManager::ReleaseSegment(int segmentId) {
    RecursiveLock lock(m_mutex);
    
    for (auto& seg : m_segments) {
        if (seg.id == segmentId) {
            if (seg.status != SegmentStatus::Complete) {
                seg.status = SegmentStatus::Pending;
            }
            seg.connectionId = -1;
            seg.speed = 0;
            return;
        }
    }
}

// ─── Get Segment Snapshot ──────────────────────────────────────────────────
std::vector<Segment> SegmentManager::GetSegments() const {
    RecursiveLock lock(m_mutex);
    return m_segments;
}

std::vector<SegmentInfo> SegmentManager::ToSegmentInfoVector() const {
    RecursiveLock lock(m_mutex);
    
    std::vector<SegmentInfo> result;
    result.reserve(m_segments.size());
    
    for (const auto& seg : m_segments) {
        SegmentInfo info;
        info.startByte = seg.startByte;
        info.endByte = seg.endByte;
        info.downloadedBytes = seg.DownloadedBytes();
        info.connectionId = seg.connectionId;
        info.complete = (seg.status == SegmentStatus::Complete);
        result.push_back(info);
    }
    
    return result;
}

// ─── Progress Queries ──────────────────────────────────────────────────────
int64 SegmentManager::GetTotalDownloaded() const {
    RecursiveLock lock(m_mutex);
    
    int64 total = 0;
    for (const auto& seg : m_segments) {
        total += seg.DownloadedBytes();
    }
    return total;
}

int64 SegmentManager::GetFileSize() const {
    RecursiveLock lock(m_mutex);
    return m_fileSize;
}

double SegmentManager::GetOverallProgress() const {
    RecursiveLock lock(m_mutex);
    
    if (m_fileSize <= 0) return 0.0;
    return static_cast<double>(GetTotalDownloaded()) / m_fileSize * 100.0;
}

bool SegmentManager::IsComplete() const {
    RecursiveLock lock(m_mutex);
    
    for (const auto& seg : m_segments) {
        if (seg.status != SegmentStatus::Complete) return false;
    }
    return !m_segments.empty();
}

int SegmentManager::GetActiveConnectionCount() const {
    RecursiveLock lock(m_mutex);
    
    int count = 0;
    for (const auto& seg : m_segments) {
        if (seg.status == SegmentStatus::Active && seg.connectionId >= 0) {
            count++;
        }
    }
    return count;
}

int SegmentManager::GetSegmentCount() const {
    RecursiveLock lock(m_mutex);
    return static_cast<int>(m_segments.size());
}

void SegmentManager::SetMaxConnections(int maxConn) {
    RecursiveLock lock(m_mutex);
    m_maxConnections = std::clamp(maxConn, constants::MIN_CONNECTIONS, constants::MAX_CONNECTIONS);
}

int SegmentManager::GetMaxConnections() const {
    RecursiveLock lock(m_mutex);
    return m_maxConnections;
}

// ─── Find Largest Active Segment ───────────────────────────────────────────
int SegmentManager::FindLargestActiveSegment() const {
    // No lock needed - caller holds m_mutex
    
    int bestIdx = -1;
    int64 bestRemaining = 0;
    
    for (size_t i = 0; i < m_segments.size(); ++i) {
        const auto& seg = m_segments[i];
        if (seg.status == SegmentStatus::Active) {
            int64 remaining = seg.RemainingBytes();
            if (remaining > bestRemaining) {
                bestRemaining = remaining;
                bestIdx = static_cast<int>(i);
            }
        }
    }
    
    return bestIdx;
}

// ─── Find First Pending Segment ───────────────────────────────────────────
int SegmentManager::FindPendingSegment() const {
    // No lock needed - caller holds m_mutex
    
    for (size_t i = 0; i < m_segments.size(); ++i) {
        if (m_segments[i].status == SegmentStatus::Pending ||
            m_segments[i].status == SegmentStatus::Error) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// ─── Split Segment ─────────────────────────────────────────────────────────
int SegmentManager::SplitSegment(int segmentIdx) {
    // No lock needed - caller holds m_mutex
    
    if (segmentIdx < 0 || segmentIdx >= static_cast<int>(m_segments.size())) {
        return -1;
    }
    
    auto& parent = m_segments[segmentIdx];
    
    // Calculate split point: midpoint of the REMAINING bytes
    // (not the total segment, since some may already be downloaded)
    int64 remaining = parent.RemainingBytes();
    if (remaining < m_minSegmentSize * 2) {
        return -1;  // Too small to split
    }
    
    int64 splitPoint = parent.currentPos + (remaining / 2);
    
    // Align to BUFFER_SIZE boundary for I/O efficiency
    splitPoint = (splitPoint / constants::BUFFER_SIZE) * constants::BUFFER_SIZE;
    if (splitPoint <= parent.currentPos) {
        splitPoint = parent.currentPos + m_minSegmentSize;
    }
    if (splitPoint > parent.endByte - m_minSegmentSize) {
        return -1;  // Would create too-small second half
    }
    
    // Create new segment for the second half
    Segment newSeg;
    newSeg.id = m_nextSegmentId++;
    newSeg.startByte = splitPoint;
    newSeg.endByte = parent.endByte;
    newSeg.currentPos = splitPoint;
    newSeg.connectionId = -1;
    newSeg.status = SegmentStatus::Pending;
    newSeg.speed = 0;
    
    // Trim the parent segment to the first half
    parent.endByte = splitPoint - 1;
    
    // Insert the new segment right after the parent (maintain order)
    m_segments.insert(m_segments.begin() + segmentIdx + 1, newSeg);
    
    LOG_DEBUG(L"SegmentManager: split segment %d at byte %lld -> new segment %d "
              L"(parent: %lld-%lld, child: %lld-%lld)",
              parent.id, splitPoint, newSeg.id,
              parent.startByte, parent.endByte,
              newSeg.startByte, newSeg.endByte);
    
    return newSeg.id;
}

// ─── State Persistence ─────────────────────────────────────────────────────
bool SegmentManager::SaveState(const String& filePath) const {
    RecursiveLock lock(m_mutex);
    
    try {
        std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) return false;
        
        // Header
        uint32 magic = 0x53454749; // "SEGI"
        uint32 version = 1;
        int64 fileSize = m_fileSize;
        uint32 segCount = static_cast<uint32>(m_segments.size());
        
        file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
        file.write(reinterpret_cast<const char*>(&version), sizeof(version));
        file.write(reinterpret_cast<const char*>(&fileSize), sizeof(fileSize));
        file.write(reinterpret_cast<const char*>(&segCount), sizeof(segCount));
        
        // Segments
        for (const auto& seg : m_segments) {
            file.write(reinterpret_cast<const char*>(&seg.id), sizeof(seg.id));
            file.write(reinterpret_cast<const char*>(&seg.startByte), sizeof(seg.startByte));
            file.write(reinterpret_cast<const char*>(&seg.endByte), sizeof(seg.endByte));
            file.write(reinterpret_cast<const char*>(&seg.currentPos), sizeof(seg.currentPos));
            uint8 status = static_cast<uint8>(seg.status);
            file.write(reinterpret_cast<const char*>(&status), sizeof(status));
        }
        
        file.flush();
        return true;
    }
    catch (...) {
        return false;
    }
}

bool SegmentManager::LoadStateFromFile(const String& filePath) {
    RecursiveLock lock(m_mutex);
    
    try {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) return false;
        
        uint32 magic, version;
        int64 fileSize;
        uint32 segCount;
        
        file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        file.read(reinterpret_cast<char*>(&version), sizeof(version));
        file.read(reinterpret_cast<char*>(&fileSize), sizeof(fileSize));
        file.read(reinterpret_cast<char*>(&segCount), sizeof(segCount));
        
        if (magic != 0x53454749 || version != 1) return false;
        
        m_fileSize = fileSize;
        m_segments.clear();
        m_nextSegmentId = 0;
        
        for (uint32 i = 0; i < segCount; ++i) {
            Segment seg;
            uint8 status;
            
            file.read(reinterpret_cast<char*>(&seg.id), sizeof(seg.id));
            file.read(reinterpret_cast<char*>(&seg.startByte), sizeof(seg.startByte));
            file.read(reinterpret_cast<char*>(&seg.endByte), sizeof(seg.endByte));
            file.read(reinterpret_cast<char*>(&seg.currentPos), sizeof(seg.currentPos));
            file.read(reinterpret_cast<char*>(&status), sizeof(status));
            
            seg.status = static_cast<SegmentStatus>(status);
            seg.connectionId = -1;
            seg.speed = 0;
            
            // Non-complete segments become pending for resume
            if (seg.status != SegmentStatus::Complete) {
                seg.status = SegmentStatus::Pending;
            }
            
            m_segments.push_back(seg);
            m_nextSegmentId = std::max(m_nextSegmentId, seg.id + 1);
        }
        
        LOG_INFO(L"SegmentManager: loaded %u segments from state file", segCount);
        return true;
    }
    catch (...) {
        return false;
    }
}

} // namespace idm
