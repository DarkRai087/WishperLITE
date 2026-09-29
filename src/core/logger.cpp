#include "core/logger.h"

#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>

namespace localvoice {

Logger& Logger::instance() {
    static Logger s_instance;
    return s_instance;
}

Logger::~Logger() {
    if (running_.load(std::memory_order_relaxed)) {
        shutdown();
    }
}

void Logger::init(const LoggerConfig& config) {
    config_ = config;
    min_level_.store(config.min_level, std::memory_order_relaxed);

    if (config.log_to_file && !config.log_file_path.empty()) {
        // Ensure parent directory exists
        auto parent = std::filesystem::path(config.log_file_path).parent_path();
        if (!parent.empty()) {
            std::filesystem::create_directories(parent);
        }
        file_.open(config.log_file_path, std::ios::app);
        if (!file_.is_open()) {
            std::cerr << "[Logger] WARNING: Failed to open log file: "
                      << config.log_file_path << "\n";
        }
    }

    running_.store(true, std::memory_order_release);
    writer_thread_ = std::thread(&Logger::writer_thread_func, this);

    // Set writer thread to low priority — it must never contend with audio/inference
#ifdef _WIN32
    if (writer_thread_.joinable()) {
        SetThreadPriority(writer_thread_.native_handle(), THREAD_PRIORITY_BELOW_NORMAL);
    }
#endif

    log(LogLevel::Info, "logger", std::format("Logger initialized (min_level={})",
        log_level_name(config.min_level)));
}

void Logger::shutdown() {
    log(LogLevel::Info, "logger", "Logger shutting down");

    running_.store(false, std::memory_order_release);
    queue_cv_.notify_one();

    if (writer_thread_.joinable()) {
        writer_thread_.join();
    }

    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }
}

void Logger::log(LogLevel level,
                 std::string_view component,
                 std::string_view message,
                 const std::source_location& loc) {
    if (!is_enabled(level)) return;

    LogEntry entry{
        .level     = level,
        .component = std::string(component),
        .message   = std::string(message),
        .file      = std::filesystem::path(loc.file_name()).filename().string(),
        .line      = static_cast<int>(loc.line()),
        .timestamp = Clock::now(),
    };

    {
        std::lock_guard lock(queue_mutex_);
        if (queue_.size() < config_.max_queue_size) {
            queue_.push(std::move(entry));
        }
        // If queue is full, silently drop the entry.
        // This is intentional: we never block the caller.
    }
    queue_cv_.notify_one();
}

void Logger::writer_thread_func() {
    while (true) {
        LogEntry entry;
        {
            std::unique_lock lock(queue_mutex_);
            queue_cv_.wait(lock, [this] {
                return !queue_.empty() || !running_.load(std::memory_order_acquire);
            });

            if (queue_.empty() && !running_.load(std::memory_order_acquire)) {
                break;
            }

            if (!queue_.empty()) {
                entry = std::move(queue_.front());
                queue_.pop();
            } else {
                continue;
            }
        }

        write_entry(entry);
    }

    // Drain remaining entries on shutdown
    std::lock_guard lock(queue_mutex_);
    while (!queue_.empty()) {
        write_entry(queue_.front());
        queue_.pop();
    }
}

void Logger::write_entry(const LogEntry& entry) {
    std::string json = format_json(entry);

    if (config_.log_to_console) {
        auto& stream = (entry.level >= LogLevel::Error) ? std::cerr : std::cout;
        stream << json << '\n';
    }

    if (config_.log_to_file && file_.is_open()) {
        file_ << json << '\n';
        // Flush on error/fatal to ensure crash diagnostics are written
        if (entry.level >= LogLevel::Error) {
            file_.flush();
        }
    }
}

std::string Logger::format_json(const LogEntry& entry) const {
    // Format timestamp as ISO 8601
    auto sys_time = std::chrono::system_clock::now() +
        (entry.timestamp - Clock::now());
    auto time_t_val = std::chrono::system_clock::to_time_t(sys_time);
    auto ms = std::chrono::duration_cast<Milliseconds>(
        entry.timestamp.time_since_epoch()) % 1000;

    std::tm tm_buf{};
#ifdef _WIN32
    localtime_s(&tm_buf, &time_t_val);
#else
    localtime_r(&time_t_val, &tm_buf);
#endif

    // Manual JSON construction — no dependency needed, and we control escaping
    std::ostringstream oss;
    oss << "{\"ts\":\"" << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%S")
        << "." << std::setfill('0') << std::setw(3) << ms.count() << "\""
        << ",\"level\":\"" << log_level_name(entry.level) << "\""
        << ",\"component\":\"" << entry.component << "\""
        << ",\"msg\":\"";

    // Escape message for JSON
    for (char c : entry.message) {
        switch (c) {
            case '"':  oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\n': oss << "\\n";  break;
            case '\r': oss << "\\r";  break;
            case '\t': oss << "\\t";  break;
            default:   oss << c;      break;
        }
    }

    oss << "\",\"file\":\"" << entry.file << "\""
        << ",\"line\":" << entry.line
        << "}";

    return oss.str();
}

} // namespace localvoice
