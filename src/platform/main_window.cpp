#include "platform/main_window.h"
#include "core/logger.h"

#include <format>

namespace localvoice {

static constexpr const wchar_t* kWindowClass = L"LocalVoiceWindow";
static constexpr const wchar_t* kWindowTitle = L"LocalVoice — Transcript";

MainWindow::MainWindow() = default;

MainWindow::~MainWindow() {
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
}

VoidResult MainWindow::create(int width, int height) {
    HINSTANCE hInstance = GetModuleHandle(nullptr);

    // Register window class
    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = &MainWindow::wnd_proc;
    wc.hInstance      = hInstance;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(30, 30, 30));  // Dark background
    wc.lpszClassName = kWindowClass;
    wc.hIcon         = LoadIcon(nullptr, IDI_APPLICATION);

    if (!RegisterClassExW(&wc)) {
        // Class may already be registered — not a fatal error
        if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            return std::unexpected(Error{ErrorCode::WindowCreationFailed,
                std::format("RegisterClassEx failed: {}", GetLastError())});
        }
    }

    hwnd_ = CreateWindowExW(
        WS_EX_TOOLWINDOW,       // Don't show in taskbar
        kWindowClass,
        kWindowTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        nullptr, nullptr,
        hInstance,
        this                     // Pass 'this' for WM_CREATE
    );

    if (!hwnd_) {
        return std::unexpected(Error{ErrorCode::WindowCreationFailed,
            std::format("CreateWindowEx failed: {}", GetLastError())});
    }

    LV_INFO("ui", std::format("Window created: {}x{}", width, height));
    return {};
}

void MainWindow::show(bool visible) {
    if (hwnd_) {
        ShowWindow(hwnd_, visible ? SW_SHOW : SW_HIDE);
    }
}

void MainWindow::append_text(const std::string& text) {
    {
        std::lock_guard lock(text_mutex_);
        if (!display_text_.empty()) {
            display_text_ += "\r\n";
        }
        display_text_ += text;

        // Keep display text bounded
        if (display_text_.size() > 8192) {
            auto pos = display_text_.find("\r\n", display_text_.size() - 4096);
            if (pos != std::string::npos) {
                display_text_ = display_text_.substr(pos + 2);
            }
        }
    }

    // Post a message to trigger repaint on the UI thread
    if (hwnd_) {
        PostMessage(hwnd_, WM_APPEND_TEXT, 0, 0);
    }
}

void MainWindow::set_status(AppState state) {
    current_state_ = state;
    if (hwnd_) {
        PostMessage(hwnd_, WM_UPDATE_STATUS, 0, 0);
    }
}

LRESULT CALLBACK MainWindow::wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MainWindow* self = nullptr;

    if (msg == WM_CREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        self = static_cast<MainWindow*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (self) {
        return self->handle_message(msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT MainWindow::handle_message(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT:
        on_paint();
        return 0;

    case WM_APPEND_TEXT:
    case WM_UPDATE_STATUS:
        InvalidateRect(hwnd_, nullptr, TRUE);
        return 0;

    case WM_CLOSE:
        // Hide instead of destroy — the tray icon keeps the app running
        show(false);
        return 0;

    case WM_DESTROY:
        return 0;

    default:
        return DefWindowProc(hwnd_, msg, wParam, lParam);
    }
}

void MainWindow::on_paint() {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd_, &ps);

    // Dark theme colors
    SetBkColor(hdc, RGB(30, 30, 30));
    SetTextColor(hdc, RGB(220, 220, 220));

    // Fill background
    RECT client;
    GetClientRect(hwnd_, &client);
    HBRUSH bg = CreateSolidBrush(RGB(30, 30, 30));
    FillRect(hdc, &client, bg);
    DeleteObject(bg);

    // Draw status bar at top
    RECT status_rect = client;
    status_rect.bottom = 24;

    const wchar_t* status_text;
    COLORREF status_color;
    switch (current_state_) {
    case AppState::Idle:
        status_text = L"● Idle";
        status_color = RGB(128, 128, 128);
        break;
    case AppState::Listening:
        status_text = L"● Listening";
        status_color = RGB(0, 200, 100);
        break;
    case AppState::Transcribing:
        status_text = L"● Transcribing...";
        status_color = RGB(255, 180, 0);
        break;
    case AppState::Error:
        status_text = L"● Error";
        status_color = RGB(255, 60, 60);
        break;
    default:
        status_text = L"● LocalVoice";
        status_color = RGB(100, 150, 255);
        break;
    }

    HBRUSH status_bg = CreateSolidBrush(RGB(40, 40, 40));
    FillRect(hdc, &status_rect, status_bg);
    DeleteObject(status_bg);

    SetTextColor(hdc, status_color);
    HFONT font = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HFONT old_font = static_cast<HFONT>(SelectObject(hdc, font));

    DrawTextW(hdc, status_text, -1, &status_rect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    // Draw transcript text
    SetTextColor(hdc, RGB(220, 220, 220));
    RECT text_rect = client;
    text_rect.top = 28;
    text_rect.left += 4;
    text_rect.right -= 4;

    std::string text;
    {
        std::lock_guard lock(text_mutex_);
        text = display_text_;
    }

    if (!text.empty()) {
        // Convert to wide string for DrawText
        int wlen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(),
            static_cast<int>(text.size()), nullptr, 0);
        std::wstring wtext(wlen, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, text.c_str(),
            static_cast<int>(text.size()), wtext.data(), wlen);

        DrawTextW(hdc, wtext.c_str(), -1, &text_rect,
            DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX);
    }

    SelectObject(hdc, old_font);
    DeleteObject(font);

    EndPaint(hwnd_, &ps);
}

} // namespace localvoice
