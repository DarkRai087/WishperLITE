#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// Performance diagnostics — memory, CPU, latency tracking
// ─────────────────────────────────────────────────────────────────────────────

#include "core/types.h"

#include <atomic>
#include <string>
#include <mutex>

namespace localvoice {

/// Memory usage breakdown (in bytes)
struct MemoryReport {
    size_t application = 0;   ///< Code + stack + general heap
    size_t audio       = 0;   ///< Audio buffers
    size_t vad         = 0;   ///< VAD model + buffers
    size_t stt         = 0;   ///< Whisper model + inference scratch
    size_t ui          = 0;   ///< UI resources
    size_t total       = 0;   ///< Total process RSS
};

/// Latency statistics
struct LatencyStats {
    double p50_ms = 0.0;
    double p95_ms = 0.0;
    double p99_ms = 0.0;
    double min_ms = 0.0;
    double max_ms = 0.0;
    size_t sample_count = 0;
};

/// Performance counters
struct PerfCounters {
    std::atomic<uint64_t> total_inferences{0};
    std::atomic<uint64_t> total_audio_frames{0};
    std::atomic<uint64_t> total_speech_segments{0};
    std::atomic<uint64_t> dropped_audio_frames{0};
    std::atomic<uint64_t> vad_activations{0};
};

/// Diagnostics subsystem
class Diagnostics {
public:
    static Diagnostics& instance();

    /// Get current process memory usage
    MemoryReport get_memory_report() const;

    /// Get current CPU usage percentage (approximate)
    double get_cpu_usage_percent() const;

    /// Record an inference latency measurement
    void record_inference_latency(Duration duration);

    /// Get inference latency statistics
    LatencyStats get_inference_latency_stats() const;

    /// Get the performance counters (lock-free atomic reads)
    PerfCounters& counters() { return counters_; }

    /// Format a memory report as a human-readable string
    static std::string format_memory_report(const MemoryReport& report);

    Diagnostics(const Diagnostics&) = delete;
    Diagnostics& operator=(const Diagnostics&) = delete;

private:
    Diagnostics() = default;

    PerfCounters counters_;

    // Latency tracking — circular buffer of recent measurements
    static constexpr size_t kMaxLatencySamples = 1000;
    mutable std::mutex latency_mutex_;
    std::vector<double> latency_samples_;
    size_t latency_write_pos_ = 0;
};

} // namespace localvoice
