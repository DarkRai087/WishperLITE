// ─────────────────────────────────────────────────────────────────────────────
// LocalVoice — main entry point
// ─────────────────────────────────────────────────────────────────────────────
//
// This is a Win32 GUI application (WinMain entry point).
// It initializes COM, creates the Application, and runs the message loop.
//

#include "core/application.h"
#include "core/logger.h"

#include <localvoice/version.h>

#include <Windows.h>
#include <objbase.h>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPWSTR lpCmdLine, int nCmdShow) {
    // Suppress unused parameter warnings
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    // Initialize COM for the main thread (required for WASAPI)
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        MessageBoxW(nullptr, L"Failed to initialize COM.", L"LocalVoice Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    {
        localvoice::Application app;

        // Initialize the application
        auto result = app.init(__argc, __wargv);
        if (!result) {
            // Convert error message to wide string for MessageBox
            auto& err = result.error();
            std::wstring msg(err.message.begin(), err.message.end());
            MessageBoxW(nullptr, msg.c_str(), L"LocalVoice — Initialization Error",
                        MB_OK | MB_ICONERROR);
            CoUninitialize();
            return 1;
        }

        // Run the message loop
        int exit_code = app.run();

        // Clean shutdown
        app.request_shutdown();

        CoUninitialize();
        return exit_code;
    }
}
