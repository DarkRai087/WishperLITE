#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// Structured Logger — JSON Lines format, async file writer
// ─────────────────────────────────────────────────────────────────────────────

#include "core/types.h"

#include <string>
#include <string_view>
#include <source_location>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <fstream>
#include <atomic>
#include <format>

namespace localvoice {

/// Log severity levels
enum class LogLevel : uint8_t {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Fatal,
};

/// Convert LogLevel to string
constexpr std::string_view log_level_name(LogLevel level) {
    switch (level) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Fatal: return "FATAL";
        default:              return "UNKNOWN";
    }
}

/// Configuration for the logger
struct LoggerConfig {
    std::string log_file_path;                       ///< Path to log file (empty = no file)
    LogLevel    min_level       = LogLevel::Info;     ///< Minimum level to log
    bool        log_to_console  = true;               ///< Also write to stdout/stderr
    bool        log_to_file     = false;              ///< Write to file
    size_t      max_queue_size  = 4096;               ///< Max queued log entries before dropping
};

/// A single structured log entry
struct LogEntry {
    LogLevel    level;
    std::string component;    ///< Module name (e.g., "audio", "stt", "vad")
    std::string message;
    std::string file;
    int         line = 0;
    TimePoint   timestamp;
};

/// Async structured logger
///
/// The logger queues entries and writes them from a dedicated low-priority
/// background thread. This ensures logging never blocks the audio or
/// inference paths.
///
/// Format: JSON Lines (one JSON object per line)
class Logger {
public:
    /// Get the singleton logger instance
    static Logger& instance();

    /// Initialize the logger with the given configuration.
    /// Must be called once at application startup.
    void init(const LoggerConfig& config);

    /// Shut down the logger, flushing all pending entries.
    /// Blocks until the write queue is drained.
    void shutdown();

    /// Submit a log entry. Lock-free fast path when queue is not full.
    void log(LogLevel level,
             std::string_view component,
             std::string_view message,
             const std::source_location& loc = std::source_location::current());

    /// Check if a given level would be logged (to avoid formatting overhead)
    [[nodiscard]] bool is_enabled(LogLevel level) const noexcept {
        return level >= min_level_.load(std::memory_order_relaxed);
    }

    /// Change minimum log level at runtime
    void set_min_level(LogLevel level) noexcept {
        min_level_.store(level, std::memory_order_relaxed);
    }

    // Non-copyable, non-movable
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    Logger() = default;
    ~Logger();

    void writer_thread_func();
    void write_entry(const LogEntry& entry);
    std::string format_json(const LogEntry& entry) const;

    LoggerConfig              config_{};
    std::atomic<LogLevel>     min_level_{LogLevel::Info};
    std::atomic<bool>         running_{false};

    // Writer thread and queue
    std::thread               writer_thread_;
    std::queue<LogEntry>      queue_;
    std::mutex                queue_mutex_;
    std::condition_variable   queue_cv_;

    // File output
    std::ofstream             file_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Convenience macros — these are intentionally macros to capture source_location
// ─────────────────────────────────────────────────────────────────────────────

#define LV_LOG(level, component, msg) \
    do { \
        if (::localvoice::Logger::instance().is_enabled(level)) { \
            ::localvoice::Logger::instance().log(level, component, msg); \
        } \
    } while (0)

#define LV_TRACE(component, msg) LV_LOG(::localvoice::LogLevel::Trace, component, msg)
#define LV_DEBUG(component, msg) LV_LOG(::localvoice::LogLevel::Debug, component, msg)
#define LV_INFO(component, msg)  LV_LOG(::localvoice::LogLevel::Info,  component, msg)
#define LV_WARN(component, msg)  LV_LOG(::localvoice::LogLevel::Warn,  component, msg)
#define LV_ERROR(component, msg) LV_LOG(::localvoice::LogLevel::Error, component, msg)
#define LV_FATAL(component, msg) LV_LOG(::localvoice::LogLevel::Fatal, component, msg)

} // namespace localvoice
