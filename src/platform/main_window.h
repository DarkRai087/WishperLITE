#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// Main Window — minimal Win32 transcript display
// ─────────────────────────────────────────────────────────────────────────────

#include "core/types.h"

#include <string>
#include <mutex>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace localvoice {

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    /// Create the window. Call from the UI thread.
    VoidResult create(int width, int height);

    /// Show or hide the window
    void show(bool visible);

    /// Append transcribed text (thread-safe — posts message to UI thread)
    void append_text(const std::string& text);

    /// Update the status display
    void set_status(AppState state);

    /// Get the window handle
    [[nodiscard]] HWND hwnd() const noexcept { return hwnd_; }

private:
    static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT handle_message(UINT msg, WPARAM wParam, LPARAM lParam);
    void on_paint();

    HWND        hwnd_ = nullptr;
    std::mutex  text_mutex_;
    std::string display_text_;
    AppState    current_state_ = AppState::Idle;

    // Custom message IDs
    static constexpr UINT WM_APPEND_TEXT = WM_USER + 1;
    static constexpr UINT WM_UPDATE_STATUS = WM_USER + 2;
};

} // namespace localvoice
