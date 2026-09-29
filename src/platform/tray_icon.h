#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// System Tray Icon — notification area with context menu
// ─────────────────────────────────────────────────────────────────────────────

#include "core/types.h"

#include <functional>

#ifdef _WIN32
#include <Windows.h>
#include <shellapi.h>
#endif

namespace localvoice {

/// Tray menu item IDs
enum class TrayMenuId : UINT {
    ShowWindow  = 1001,
    Start       = 1002,
    Stop        = 1003,
    PushToTalk  = 1004,
    Diagnostics = 1005,
    Exit        = 1006,
};

/// Callback for tray menu item selection
using TrayMenuCallback = std::function<void(TrayMenuId id)>;

class TrayIcon {
public:
    TrayIcon();
    ~TrayIcon();

    /// Create the tray icon. Requires a window handle for messages.
    VoidResult create(HWND owner_hwnd, TrayMenuCallback callback);

    /// Remove the tray icon
    void remove();

    /// Update the tooltip text
    void set_tooltip(const std::wstring& text);

    /// Show a balloon notification
    void show_notification(const std::wstring& title, const std::wstring& message);

    /// Handle tray-related window messages. Returns true if handled.
    bool handle_message(UINT msg, WPARAM wParam, LPARAM lParam);

private:
    void show_context_menu();

    HWND              owner_hwnd_ = nullptr;
    NOTIFYICONDATAW   nid_{};
    bool              created_ = false;
    TrayMenuCallback  callback_;

    static constexpr UINT WM_TRAY_ICON = WM_USER + 100;
    static constexpr UINT kTrayIconId = 1;
};

} // namespace localvoice
