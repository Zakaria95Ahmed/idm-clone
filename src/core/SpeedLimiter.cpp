/**
 * @file SpeedLimiter.cpp
 * @brief Token bucket speed limiter implementation
 */

#include "stdafx.h"
#include "SpeedLimiter.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace idm {

SpeedLimiter& SpeedLimiter::Instance() {
    static SpeedLimiter instance;
    return instance;
}

void SpeedLimiter::SetLimit(int64 bytesPerSecond) {
    Lock lock(m_mutex);
    m_limitBps.store(bytesPerSecond);
    m_burstCapacity = static_cast<double>(bytesPerSecond * 2);
    m_tokens = m_burstCapacity;
    m_lastRefill = Clock::now();
}

size_t SpeedLimiter::RequestBytes(size_t bytes) {
    if (!IsActive() || bytes == 0) return bytes;
    
    Lock lock(m_mutex);
    
    int64 limit = m_limitBps.load();
    if (limit <= 0) return bytes;
    
    auto now = Clock::now();
    double elapsed = std::chrono::duration<double>(now - m_lastRefill).count();
    m_lastRefill = now;
    
    m_tokens += elapsed * limit;
    if (m_tokens > m_burstCapacity) {
        m_tokens = m_burstCapacity;
    }
    
    if (m_tokens >= static_cast<double>(bytes)) {
        m_tokens -= static_cast<double>(bytes);
        return bytes;
    }
    
    if (m_tokens > 0) {
        size_t permitted = static_cast<size_t>(m_tokens);
        m_tokens = 0;
        if (permitted > 0) return permitted;
    }
    
    double neededTokens = static_cast<double>(bytes);
    double sleepTime = neededTokens / limit;
    
    sleepTime = sleepTime < 0.1 ? sleepTime : 0.1;
    
    lock.unlock();
    std::this_thread::sleep_for(std::chrono::milliseconds(
        static_cast<int>(sleepTime * 1000)));
    lock.lock();
    
    auto afterSleep = Clock::now();
    double sleptFor = std::chrono::duration<double>(afterSleep - m_lastRefill).count();
    m_lastRefill = afterSleep;
    
    m_tokens += sleptFor * limit;
    if (m_tokens > m_burstCapacity) m_tokens = m_burstCapacity;
    
    double minVal = m_tokens < static_cast<double>(bytes) ? m_tokens : static_cast<double>(bytes);
    size_t permitted = static_cast<size_t>(minVal);
    m_tokens -= static_cast<double>(permitted);
    
    return permitted > 0 ? permitted : 1;
}

void SpeedLimiter::Reset() {
    Lock lock(m_mutex);
    m_tokens = m_burstCapacity;
    m_lastRefill = Clock::now();
}

} // namespace idm
