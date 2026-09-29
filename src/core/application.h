#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// Application lifecycle — init, run, shutdown
// ─────────────────────────────────────────────────────────────────────────────

#include "core/types.h"
#include "core/config.h"
#include "core/event_bus.h"

#include <memory>
#include <atomic>

namespace localvoice {

class Logger;

/// Main application class — owns all subsystems
class Application {
public:
    Application();
    ~Application();

    /// Initialize all subsystems. Must be called before run().
    VoidResult init(int argc, wchar_t* argv[]);

    /// Run the application message loop. Blocks until shutdown.
    int run();

    /// Request a clean shutdown.
    void request_shutdown();

    /// Get current application state
    [[nodiscard]] AppState state() const noexcept {
        return state_.load(std::memory_order_acquire);
    }

    /// Get the event bus
    [[nodiscard]] EventBus& event_bus() noexcept { return event_bus_; }

    /// Get the configuration
    [[nodiscard]] const AppConfig& config() const noexcept { return config_; }
    [[nodiscard]] AppConfig& config() noexcept { return config_; }

    // Non-copyable
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

private:
    void set_state(AppState new_state);
    VoidResult init_logging();
    VoidResult init_config(int argc, wchar_t* argv[]);

    std::atomic<AppState> state_{AppState::Uninitialized};
    AppConfig             config_;
    EventBus              event_bus_;
    std::string           config_path_;
};

} // namespace localvoice
