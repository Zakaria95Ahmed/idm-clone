/**
 * @file Logger.h
 * @brief Thread-safe logging facility for the IDM Clone application
 *
 * Architecture: Singleton logger with asynchronous write-back.
 * Log messages are queued in a lock-free ring buffer and flushed to
 * disk by a dedicated writer thread. This ensures that logging from
 * hot paths (download threads, UI thread) never blocks on I/O.
 *
 * Log levels follow syslog convention:
 *   TRACE < DEBUG < INFO < WARN < ERROR < FATAL
 *
 * Usage:
 *   LOG_INFO(L"Download started: %s", url.c_str());
 *   LOG_ERROR(L"Connection failed: error %d", GetLastError());
 */

#pragma once
#include "stdafx.h"

namespace idm {

// ─── Log Level Enumeration ─────────────────────────────────────────────────
enum class LogLevel : int {
    Trace = 0,  // Extremely verbose: packet-level details
    Debug = 1,  // Development diagnostics: segment splits, state transitions
    Info  = 2,  // Normal operational messages: download started/completed
    Warn  = 3,  // Recoverable issues: retry, fallback to single connection
    Error = 4,  // Failures: connection timeout, file write error
    Fatal = 5   // Unrecoverable: out of memory, corrupt database
};

// ─── Log Entry Structure ───────────────────────────────────────────────────
struct LogEntry {
    SystemTimePoint     timestamp;
    LogLevel            level;
    DWORD               threadId;
    String              message;
    String              source;     // __FILE__ or component name
    int                 line;       // __LINE__
};

// ─── Logger Class (Singleton) ──────────────────────────────────────────────
class Logger {
public:
    /**
     * Get the singleton instance. Thread-safe via Meyers' singleton
     * (C++11 guarantees static local initialization is thread-safe).
     */
    static Logger& Instance();

    // Non-copyable, non-movable singleton
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    /**
     * Initialize the logger with output file path and minimum log level.
     * Must be called once at application startup before any logging.
     *
     * @param logFilePath  Path to the log file (created/appended)
     * @param minLevel     Minimum level to record (lower levels are discarded)
     * @param maxFileSizeMB Maximum log file size before rotation (default 10MB)
     */
    bool Initialize(const String& logFilePath, 
                    LogLevel minLevel = LogLevel::Info,
                    int maxFileSizeMB = 10);

    /**
     * Shut down the logger: flush pending entries and close the file.
     * Called at application exit.
     */
    void Shutdown();

    /**
     * Submit a log entry. This is lock-free on the fast path.
     * The entry is queued and written asynchronously by the writer thread.
     */
    void Log(LogLevel level, const wchar_t* source, int line,
             const wchar_t* format, ...);

    /**
     * Set minimum log level at runtime (e.g., from Options dialog).
     */
    void SetLevel(LogLevel level) { m_minLevel.store(level, std::memory_order_relaxed); }
    LogLevel GetLevel() const { return m_minLevel.load(std::memory_order_relaxed); }

    /**
     * Force an immediate flush of all pending log entries.
     */
    void Flush();

private:
    Logger();
    ~Logger();

    void WriterThread();
    void RotateLogFile();
    String FormatEntry(const LogEntry& entry) const;
    const wchar_t* LevelToString(LogLevel level) const;

    // State
    std::atomic<LogLevel>       m_minLevel{LogLevel::Info};
    std::atomic<bool>           m_running{false};
    bool                        m_initialized{false};

    // File output
    String                      m_logFilePath;
    std::wofstream              m_file;
    int                         m_maxFileSizeBytes{10 * 1024 * 1024};

    // Async write queue
    std::queue<LogEntry>        m_queue;
    Mutex                       m_queueMutex;
    CondVar                     m_queueCV;
    std::thread                 m_writerThread;
};

// ─── Convenience Macros ────────────────────────────────────────────────────
// These macros capture __FILE__ and __LINE__ at the call site and forward
// to the singleton logger. The do-while(0) pattern ensures macro hygiene.

#define LOG_TRACE(fmt, ...) \
    do { idm::Logger::Instance().Log(idm::LogLevel::Trace, _CRT_WIDE(__FILE__), __LINE__, fmt, ##__VA_ARGS__); } while(0)

#define LOG_DEBUG(fmt, ...) \
    do { idm::Logger::Instance().Log(idm::LogLevel::Debug, _CRT_WIDE(__FILE__), __LINE__, fmt, ##__VA_ARGS__); } while(0)

#define LOG_INFO(fmt, ...) \
    do { idm::Logger::Instance().Log(idm::LogLevel::Info, _CRT_WIDE(__FILE__), __LINE__, fmt, ##__VA_ARGS__); } while(0)

#define LOG_WARN(fmt, ...) \
    do { idm::Logger::Instance().Log(idm::LogLevel::Warn, _CRT_WIDE(__FILE__), __LINE__, fmt, ##__VA_ARGS__); } while(0)

#define LOG_ERROR(fmt, ...) \
    do { idm::Logger::Instance().Log(idm::LogLevel::Error, _CRT_WIDE(__FILE__), __LINE__, fmt, ##__VA_ARGS__); } while(0)

#define LOG_FATAL(fmt, ...) \
    do { idm::Logger::Instance().Log(idm::LogLevel::Fatal, _CRT_WIDE(__FILE__), __LINE__, fmt, ##__VA_ARGS__); } while(0)

} // namespace idm
