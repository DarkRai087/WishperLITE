#include "platform/tray_icon.h"
#include "core/logger.h"

namespace localvoice {

TrayIcon::TrayIcon() = default;

TrayIcon::~TrayIcon() {
    remove();
}

VoidResult TrayIcon::create(HWND owner_hwnd, TrayMenuCallback callback) {
    owner_hwnd_ = owner_hwnd;
    callback_   = std::move(callback);

    nid_.cbSize           = sizeof(NOTIFYICONDATAW);
    nid_.hWnd             = owner_hwnd_;
    nid_.uID              = kTrayIconId;
    nid_.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid_.uCallbackMessage = WM_TRAY_ICON;
    nid_.hIcon            = LoadIcon(nullptr, IDI_APPLICATION);
    wcscpy_s(nid_.szTip, L"LocalVoice — Idle");

    if (!Shell_NotifyIconW(NIM_ADD, &nid_)) {
        return std::unexpected(Error{ErrorCode::TrayIconFailed,
            "Failed to create system tray icon"});
    }

    // Set version for modern balloon support
    nid_.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &nid_);

    created_ = true;
    LV_INFO("tray", "System tray icon created");
    return {};
}

void TrayIcon::remove() {
    if (created_) {
        Shell_NotifyIconW(NIM_DELETE, &nid_);
        created_ = false;
        LV_INFO("tray", "System tray icon removed");
    }
}

void TrayIcon::set_tooltip(const std::wstring& text) {
    if (!created_) return;
    wcsncpy_s(nid_.szTip, text.c_str(), _TRUNCATE);
    nid_.uFlags = NIF_TIP;
    Shell_NotifyIconW(NIM_MODIFY, &nid_);
}

void TrayIcon::show_notification(const std::wstring& title, const std::wstring& message) {
    if (!created_) return;

    nid_.uFlags = NIF_INFO;
    wcsncpy_s(nid_.szInfoTitle, title.c_str(), _TRUNCATE);
    wcsncpy_s(nid_.szInfo, message.c_str(), _TRUNCATE);
    nid_.dwInfoFlags = NIIF_INFO;
    Shell_NotifyIconW(NIM_MODIFY, &nid_);
}

bool TrayIcon::handle_message(UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg != WM_TRAY_ICON) return false;

    switch (LOWORD(lParam)) {
    case WM_RBUTTONUP:
    case WM_CONTEXTMENU:
        show_context_menu();
        return true;

    case WM_LBUTTONDBLCLK:
        if (callback_) callback_(TrayMenuId::ShowWindow);
        return true;
    }

    return false;
}

void TrayIcon::show_context_menu() {
    HMENU menu = CreatePopupMenu();
    if (!menu) return;

    AppendMenuW(menu, MF_STRING, static_cast<UINT>(TrayMenuId::ShowWindow),  L"Show &Window");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, static_cast<UINT>(TrayMenuId::Start),       L"&Start Transcription");
    AppendMenuW(menu, MF_STRING, static_cast<UINT>(TrayMenuId::Stop),        L"S&top Transcription");
    AppendMenuW(menu, MF_STRING, static_cast<UINT>(TrayMenuId::PushToTalk),  L"&Push-to-Talk");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, static_cast<UINT>(TrayMenuId::Diagnostics), L"&Diagnostics");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, static_cast<UINT>(TrayMenuId::Exit),        L"E&xit");

    POINT pt;
    GetCursorPos(&pt);

    // Required for the menu to close when clicking outside
    SetForegroundWindow(owner_hwnd_);

    UINT cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY,
        pt.x, pt.y, 0, owner_hwnd_, nullptr);

    DestroyMenu(menu);

    if (cmd != 0 && callback_) {
        callback_(static_cast<TrayMenuId>(cmd));
    }
}

} // namespace localvoice
