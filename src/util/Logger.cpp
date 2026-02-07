/**
 * @file Logger.cpp
 * @brief Implementation of the thread-safe asynchronous Logger
 *
 * The writer thread wakes up when entries are queued or every 100ms
 * (whichever comes first) to batch-write log entries to disk. This
 * amortizes I/O syscall overhead while maintaining near-real-time
 * log visibility.
 *
 * Log rotation: when the file exceeds the configured maximum size,
 * the current file is renamed with a timestamp suffix and a new
 * file is started. This prevents unbounded log growth.
 */

#include "stdafx.h"
#include "Logger.h"
#include <cstdarg>

namespace idm {

// ─── Singleton Access ──────────────────────────────────────────────────────
Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

Logger::Logger() = default;

Logger::~Logger() {
    Shutdown();
}

// ─── Initialization ────────────────────────────────────────────────────────
bool Logger::Initialize(const String& logFilePath, LogLevel minLevel, int maxFileSizeMB) {
    if (m_initialized) return true;
    
    m_logFilePath = logFilePath;
    m_minLevel.store(minLevel);
    m_maxFileSizeBytes = maxFileSizeMB * 1024 * 1024;
    
    // Ensure the directory exists
    std::filesystem::path logDir = std::filesystem::path(logFilePath).parent_path();
    if (!logDir.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(logDir, ec);
    }
    
    // Open log file in append mode
    m_file.open(logFilePath, std::ios::out | std::ios::app);
    if (!m_file.is_open()) {
        return false;
    }
    
    // Start the asynchronous writer thread
    m_running.store(true);
    m_writerThread = std::thread(&Logger::WriterThread, this);
    
    m_initialized = true;
    
    // Write startup banner
    Log(LogLevel::Info, L"Logger", 0, 
        L"═══════════════════════════════════════════════════════");
    Log(LogLevel::Info, L"Logger", 0, 
        L"IDM Clone v%s - Logger initialized", constants::APP_VERSION);
    Log(LogLevel::Info, L"Logger", 0, 
        L"═══════════════════════════════════════════════════════");
    
    return true;
}

// ─── Shutdown ──────────────────────────────────────────────────────────────
void Logger::Shutdown() {
    if (!m_initialized) return;
    
    Log(LogLevel::Info, L"Logger", 0, L"Logger shutting down");
    
    // Signal writer thread to exit
    m_running.store(false);
    m_queueCV.notify_one();
    
    // Wait for writer thread to drain the queue and exit
    if (m_writerThread.joinable()) {
        m_writerThread.join();
    }
    
    // Close the file
    if (m_file.is_open()) {
        m_file.flush();
        m_file.close();
    }
    
    m_initialized = false;
}

// ─── Log Entry Submission ──────────────────────────────────────────────────
void Logger::Log(LogLevel level, const wchar_t* source, int line,
                 const wchar_t* format, ...) {
    // Fast rejection for below-threshold levels
    if (level < m_minLevel.load(std::memory_order_relaxed)) return;
    
    // Format the message using variable arguments
    wchar_t buffer[4096];
    va_list args;
    va_start(args, format);
    int written = _vsnwprintf_s(buffer, _TRUNCATE, format, args);
    va_end(args);
    
    if (written < 0) {
        buffer[4095] = L'\0';  // Truncated but safe
    }
    
    // Build the log entry
    LogEntry entry;
    entry.timestamp = SystemClock::now();
    entry.level     = level;
    entry.threadId  = ::GetCurrentThreadId();
    entry.message   = buffer;
    entry.source    = source ? source : L"";
    entry.line      = line;
    
    // Enqueue for async writing
    {
        Lock lock(m_queueMutex);
        m_queue.push(std::move(entry));
    }
    m_queueCV.notify_one();
}

// ─── Forced Flush ──────────────────────────────────────────────────────────
void Logger::Flush() {
    // Wake the writer thread and wait for queue to drain
    m_queueCV.notify_one();
    
    // Spin-wait for queue to empty (used only at critical points)
    for (int i = 0; i < 100; ++i) {
        {
            Lock lock(m_queueMutex);
            if (m_queue.empty()) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// ─── Async Writer Thread ───────────────────────────────────────────────────
void Logger::WriterThread() {
    ::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
    
    std::vector<LogEntry> batch;
    batch.reserve(64);
    
    while (m_running.load() || !m_queue.empty()) {
        // Wait for entries or timeout (100ms for periodic flush)
        {
            Lock lock(m_queueMutex);
            m_queueCV.wait_for(lock, std::chrono::milliseconds(100),
                [this] { return !m_queue.empty() || !m_running.load(); });
            
            // Drain the queue into a local batch to minimize lock hold time
            while (!m_queue.empty()) {
                batch.push_back(std::move(m_queue.front()));
                m_queue.pop();
            }
        }
        
        // Write the batch outside the lock
        if (!batch.empty() && m_file.is_open()) {
            for (const auto& entry : batch) {
                m_file << FormatEntry(entry) << L"\n";
            }
            m_file.flush();
            batch.clear();
            
            // Check if rotation is needed
            RotateLogFile();
        }
    }
}

// ─── Log File Rotation ─────────────────────────────────────────────────────
void Logger::RotateLogFile() {
    // Check file size
    auto pos = m_file.tellp();
    if (pos < 0 || static_cast<int64_t>(pos) < m_maxFileSizeBytes) return;
    
    m_file.close();
    
    // Rename current log with timestamp
    auto now = SystemClock::to_time_t(SystemClock::now());
    struct tm timeInfo;
    localtime_s(&timeInfo, &now);
    
    wchar_t suffix[64];
    wcsftime(suffix, 64, L"_%Y%m%d_%H%M%S", &timeInfo);
    
    std::filesystem::path logPath(m_logFilePath);
    String rotatedName = logPath.stem().wstring() + suffix + logPath.extension().wstring();
    std::filesystem::path rotatedPath = logPath.parent_path() / rotatedName;
    
    std::error_code ec;
    std::filesystem::rename(m_logFilePath, rotatedPath, ec);
    
    // Open a fresh log file
    m_file.open(m_logFilePath, std::ios::out | std::ios::trunc);
}

// ─── Entry Formatting ──────────────────────────────────────────────────────
String Logger::FormatEntry(const LogEntry& entry) const {
    auto timeT = SystemClock::to_time_t(entry.timestamp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        entry.timestamp.time_since_epoch()) % 1000;
    
    struct tm timeInfo;
    localtime_s(&timeInfo, &timeT);
    
    wchar_t timestamp[32];
    wcsftime(timestamp, 32, L"%Y-%m-%d %H:%M:%S", &timeInfo);
    
    wchar_t buffer[8192];
    _snwprintf_s(buffer, _TRUNCATE,
        L"[%s.%03lld] [%s] [T:%05lu] %s",
        timestamp,
        static_cast<long long>(ms.count()),
        LevelToString(entry.level),
        static_cast<unsigned long>(entry.threadId),
        entry.message.c_str());
    
    return buffer;
}

// ─── Level to String Conversion ────────────────────────────────────────────
const wchar_t* Logger::LevelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::Trace: return L"TRACE";
        case LogLevel::Debug: return L"DEBUG";
        case LogLevel::Info:  return L"INFO ";
        case LogLevel::Warn:  return L"WARN ";
        case LogLevel::Error: return L"ERROR";
        case LogLevel::Fatal: return L"FATAL";
        default:              return L"?????";
    }
}

} // namespace idm
