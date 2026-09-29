#include "core/diagnostics.h"

#include <algorithm>
#include <numeric>
#include <format>
#include <cmath>

#ifdef _WIN32
#include <Windows.h>
#include <Psapi.h>
#pragma comment(lib, "psapi.lib")
#endif

namespace localvoice {

Diagnostics& Diagnostics::instance() {
    static Diagnostics s_instance;
    return s_instance;
}

MemoryReport Diagnostics::get_memory_report() const {
    MemoryReport report;

#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc{};
    pmc.cb = sizeof(pmc);
    if (GetProcessMemoryInfo(GetCurrentProcess(),
                             reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc),
                             sizeof(pmc))) {
        report.total = pmc.WorkingSetSize;
        report.application = pmc.PrivateUsage;
    }
#endif

    return report;
}

double Diagnostics::get_cpu_usage_percent() const {
#ifdef _WIN32
    // Simple snapshot-based CPU usage — not highly accurate but low overhead
    static ULARGE_INTEGER last_cpu{}, last_sys_cpu{}, last_user_cpu{};
    static bool first_call = true;

    FILETIME idle, kernel, user;
    FILETIME proc_kernel, proc_user;
    FILETIME creation, exit;

    GetSystemTimes(&idle, &kernel, &user);
    GetProcessTimes(GetCurrentProcess(), &creation, &exit, &proc_kernel, &proc_user);

    ULARGE_INTEGER now_sys, now_user, now_proc_sys, now_proc_user;
    now_sys.LowPart     = kernel.dwLowDateTime;
    now_sys.HighPart    = kernel.dwHighDateTime;
    now_user.LowPart    = user.dwLowDateTime;
    now_user.HighPart   = user.dwHighDateTime;
    now_proc_sys.LowPart  = proc_kernel.dwLowDateTime;
    now_proc_sys.HighPart = proc_kernel.dwHighDateTime;
    now_proc_user.LowPart  = proc_user.dwLowDateTime;
    now_proc_user.HighPart = proc_user.dwHighDateTime;

    if (first_call) {
        last_cpu = now_sys;
        last_sys_cpu = now_proc_sys;
        last_user_cpu = now_proc_user;
        first_call = false;
        return 0.0;
    }

    auto sys_diff = (now_sys.QuadPart - last_cpu.QuadPart) +
                    (now_user.QuadPart - last_cpu.QuadPart);
    auto proc_diff = (now_proc_sys.QuadPart - last_sys_cpu.QuadPart) +
                     (now_proc_user.QuadPart - last_user_cpu.QuadPart);

    last_cpu = now_sys;
    last_sys_cpu = now_proc_sys;
    last_user_cpu = now_proc_user;

    if (sys_diff == 0) return 0.0;
    return (static_cast<double>(proc_diff) / static_cast<double>(sys_diff)) * 100.0;
#else
    return 0.0;
#endif
}

void Diagnostics::record_inference_latency(Duration duration) {
    auto ms = std::chrono::duration<double, std::milli>(duration).count();

    std::lock_guard lock(latency_mutex_);
    if (latency_samples_.size() < kMaxLatencySamples) {
        latency_samples_.push_back(ms);
    } else {
        latency_samples_[latency_write_pos_ % kMaxLatencySamples] = ms;
    }
    latency_write_pos_++;
}

LatencyStats Diagnostics::get_inference_latency_stats() const {
    std::lock_guard lock(latency_mutex_);

    LatencyStats stats;
    if (latency_samples_.empty()) return stats;

    auto sorted = latency_samples_;
    std::sort(sorted.begin(), sorted.end());

    stats.sample_count = sorted.size();
    stats.min_ms = sorted.front();
    stats.max_ms = sorted.back();

    auto percentile = [&sorted](double p) -> double {
        size_t idx = static_cast<size_t>(std::ceil(p * sorted.size())) - 1;
        idx = std::min(idx, sorted.size() - 1);
        return sorted[idx];
    };

    stats.p50_ms = percentile(0.50);
    stats.p95_ms = percentile(0.95);
    stats.p99_ms = percentile(0.99);

    return stats;
}

std::string Diagnostics::format_memory_report(const MemoryReport& report) {
    auto to_mb = [](size_t bytes) -> double {
        return static_cast<double>(bytes) / (1024.0 * 1024.0);
    };

    return std::format(
        "────────────────────────────\n"
        " LocalVoice Memory Report\n"
        "────────────────────────────\n"
        " Application:   {:.1f} MB\n"
        " Audio:         {:.1f} MB\n"
        " VAD:           {:.1f} MB\n"
        " STT:           {:.1f} MB\n"
        " UI:            {:.1f} MB\n"
        " Total (RSS):   {:.1f} MB\n"
        "────────────────────────────\n",
        to_mb(report.application),
        to_mb(report.audio),
        to_mb(report.vad),
        to_mb(report.stt),
        to_mb(report.ui),
        to_mb(report.total)
    );
}

} // namespace localvoice
