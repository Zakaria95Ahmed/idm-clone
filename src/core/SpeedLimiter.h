/**
 * @file SpeedLimiter.h
 * @brief Bandwidth throttling for download speed control
 *
 * Uses a token bucket algorithm for smooth rate limiting:
 * - Tokens are added to the bucket at the configured rate
 * - Each byte downloaded consumes one token
 * - When the bucket is empty, downloads sleep until tokens are available
 * - Burst capacity allows short speed spikes for better utilization
 */

#pragma once
#include "stdafx.h"

namespace idm {

class SpeedLimiter {
public:
    static SpeedLimiter& Instance();
    
    /**
     * Enable speed limiting at the specified rate.
     * @param bytesPerSecond  Maximum download speed (0 = unlimited)
     */
    void SetLimit(int64 bytesPerSecond);
    
    /**
     * Get the current speed limit.
     */
    int64 GetLimit() const { return m_limitBps.load(); }
    
    /**
     * Check if speed limiting is active.
     */
    bool IsEnabled() const { return m_limitBps.load() > 0; }
    
    /**
     * Enable/disable the limiter without changing the configured rate.
     */
    void Enable(bool enabled) { m_enabled.store(enabled); }
    bool IsActive() const { return m_enabled.load() && m_limitBps.load() > 0; }
    
    /**
     * Request permission to send/receive 'bytes' bytes.
     * This method may block (sleep) if the rate limit would be exceeded.
     * Call this before writing each chunk of downloaded data.
     *
     * @param bytes  Number of bytes to be transferred
     * @return The number of bytes actually permitted (may be less for 
     *         chunk splitting across rate windows)
     */
    size_t RequestBytes(size_t bytes);
    
    /**
     * Reset the token bucket (e.g., when toggling limiter).
     */
    void Reset();
    
    /**
     * Get the current effective speed across all downloads.
     */
    double GetCurrentTotalSpeed() const { return m_currentTotalSpeed.load(); }
    void UpdateCurrentTotalSpeed(double bps) { m_currentTotalSpeed.store(bps); }
    
private:
    SpeedLimiter() = default;
    
    std::atomic<int64>  m_limitBps{0};          // Bytes per second limit
    std::atomic<bool>   m_enabled{false};
    std::atomic<double> m_currentTotalSpeed{0};
    
    // Token bucket state
    mutable Mutex       m_mutex;
    double              m_tokens{0};            // Available tokens
    TimePoint           m_lastRefill;           // Last token refill time
    double              m_burstCapacity{0};     // Max burst size
};

} // namespace idm
