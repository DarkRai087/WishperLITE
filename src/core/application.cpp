#include "core/application.h"
#include "core/logger.h"
#include "core/diagnostics.h"

#include <localvoice/version.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#include <format>

namespace localvoice {

Application::Application() = default;

Application::~Application() {
    if (state_.load(std::memory_order_acquire) != AppState::Uninitialized) {
        request_shutdown();
    }
}

VoidResult Application::init(int argc, wchar_t* argv[]) {
    set_state(AppState::Initializing);

    LV_INFO("app", std::format("LocalVoice v{} starting", Version::string));

    // Initialize configuration
    auto config_result = init_config(argc, argv);
    if (!config_result) return config_result;

    // Initialize logging with config-specified settings
    auto log_result = init_logging();
    if (!log_result) return log_result;

    // Log system information
    LV_INFO("app", std::format("Platform: Windows x64"));

#ifdef _WIN32
    SYSTEM_INFO si{};
    GetSystemInfo(&si);
    LV_INFO("app", std::format("CPU cores: {}", si.dwNumberOfProcessors));

    MEMORYSTATUSEX mem{};
    mem.dwLength = sizeof(mem);
    GlobalMemoryStatusEx(&mem);
    LV_INFO("app", std::format("Total RAM: {} MB",
        mem.ullTotalPhys / (1024 * 1024)));
#endif

    set_state(AppState::Idle);
    LV_INFO("app", "Application initialized successfully");

    return {};
}

int Application::run() {
    if (state_.load(std::memory_order_acquire) != AppState::Idle) {
        LV_ERROR("app", "Cannot run: application not in Idle state");
        return 1;
    }

    LV_INFO("app", "Entering main message loop");

    // Win32 message loop
#ifdef _WIN32
    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (state_.load(std::memory_order_acquire) == AppState::ShuttingDown) {
            break;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
#endif

    return 0;
}

void Application::request_shutdown() {
    auto prev = state_.load(std::memory_order_acquire);
    if (prev == AppState::ShuttingDown || prev == AppState::Uninitialized) {
        return;
    }

    set_state(AppState::ShuttingDown);
    LV_INFO("app", "Shutdown requested");

    // Report final diagnostics
    auto mem = Diagnostics::instance().get_memory_report();
    LV_INFO("app", Diagnostics::format_memory_report(mem));

    auto latency = Diagnostics::instance().get_inference_latency_stats();
    if (latency.sample_count > 0) {
        LV_INFO("app", std::format(
            "Inference latency: P50={:.1f}ms P95={:.1f}ms P99={:.1f}ms (n={})",
            latency.p50_ms, latency.p95_ms, latency.p99_ms, latency.sample_count));
    }

    // Shut down logger last
    Logger::instance().shutdown();

    set_state(AppState::Uninitialized);

#ifdef _WIN32
    PostQuitMessage(0);
#endif
}

void Application::set_state(AppState new_state) {
    auto old = state_.exchange(new_state, std::memory_order_acq_rel);
    if (old != new_state) {
        event_bus_.publish(AppStateChangedEvent{.previous = old, .current = new_state});
    }
}

VoidResult Application::init_config(int argc, wchar_t* argv[]) {
    // Determine config path: command-line arg or default
    config_path_ = get_default_config_path();

    // Parse command-line arguments for --config override
    for (int i = 1; i < argc; ++i) {
        std::wstring arg(argv[i]);
        if (arg == L"--config" && i + 1 < argc) {
            // Convert wchar to string
            std::wstring wpath(argv[i + 1]);
            config_path_ = std::string(wpath.begin(), wpath.end());
            break;
        }
    }

    config_ = load_config(config_path_);
    return {};
}

VoidResult Application::init_logging() {
    LoggerConfig log_config;
    log_config.log_to_console = config_.logging.log_to_console;
    log_config.log_to_file    = config_.logging.log_to_file;
    log_config.log_file_path  = config_.logging.log_file_path;

    if (log_config.log_file_path.empty() && log_config.log_to_file) {
        log_config.log_file_path = get_default_log_path();
    }

    // Parse min level string
    auto level_str = config_.logging.min_level;
    if (level_str == "trace")      log_config.min_level = LogLevel::Trace;
    else if (level_str == "debug") log_config.min_level = LogLevel::Debug;
    else if (level_str == "info")  log_config.min_level = LogLevel::Info;
    else if (level_str == "warn")  log_config.min_level = LogLevel::Warn;
    else if (level_str == "error") log_config.min_level = LogLevel::Error;
    else if (level_str == "fatal") log_config.min_level = LogLevel::Fatal;

    Logger::instance().init(log_config);
    return {};
}

} // namespace localvoice
