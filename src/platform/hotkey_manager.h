#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// Hotkey Manager — global hotkey registration via Win32 RegisterHotKey
// ─────────────────────────────────────────────────────────────────────────────

#include "core/types.h"
#include "core/config.h"

#include <functional>
#include <unordered_map>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace localvoice {

/// Hotkey action identifiers
enum class HotkeyAction : int {
    PushToTalk = 1,
    ToggleTranscription = 2,
};

using HotkeyCallback = std::function<void(HotkeyAction action, bool key_down)>;

class HotkeyManager {
public:
    HotkeyManager();
    ~HotkeyManager();

    /// Register all hotkeys. Requires a window handle for WM_HOTKEY messages.
    VoidResult register_hotkeys(HWND hwnd, const HotkeyConfig& config);

    /// Unregister all hotkeys
    void unregister_all();

    /// Set the callback for hotkey events
    void set_callback(HotkeyCallback callback) { callback_ = std::move(callback); }

    /// Handle WM_HOTKEY message. Returns true if handled.
    bool handle_hotkey(WPARAM wParam);

private:
    HWND                                  hwnd_ = nullptr;
    HotkeyCallback                        callback_;
    std::unordered_map<int, HotkeyAction> registered_hotkeys_;
};

} // namespace localvoice
