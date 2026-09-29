#include "platform/hotkey_manager.h"
#include "core/logger.h"

#include <format>

namespace localvoice {

HotkeyManager::HotkeyManager() = default;

HotkeyManager::~HotkeyManager() {
    unregister_all();
}

VoidResult HotkeyManager::register_hotkeys(HWND hwnd, const HotkeyConfig& config) {
    hwnd_ = hwnd;

    // Register push-to-talk hotkey
    if (config.push_to_talk_vk != 0) {
        int id = static_cast<int>(HotkeyAction::PushToTalk);
        if (RegisterHotKey(hwnd, id, config.push_to_talk_mods, config.push_to_talk_vk)) {
            registered_hotkeys_[id] = HotkeyAction::PushToTalk;
            LV_INFO("hotkey", std::format("Push-to-talk registered: VK=0x{:02X}",
                config.push_to_talk_vk));
        } else {
            LV_WARN("hotkey", std::format(
                "Failed to register push-to-talk (VK=0x{:02X}), error={}",
                config.push_to_talk_vk, GetLastError()));
        }
    }

    // Register toggle hotkey
    if (config.toggle_vk != 0) {
        int id = static_cast<int>(HotkeyAction::ToggleTranscription);
        if (RegisterHotKey(hwnd, id, config.toggle_mods, config.toggle_vk)) {
            registered_hotkeys_[id] = HotkeyAction::ToggleTranscription;
            LV_INFO("hotkey", std::format("Toggle transcription registered: VK=0x{:02X}",
                config.toggle_vk));
        } else {
            LV_WARN("hotkey", std::format(
                "Failed to register toggle hotkey (VK=0x{:02X}), error={}",
                config.toggle_vk, GetLastError()));
        }
    }

    return {};
}

void HotkeyManager::unregister_all() {
    for (auto& [id, action] : registered_hotkeys_) {
        UnregisterHotKey(hwnd_, id);
    }
    registered_hotkeys_.clear();
}

bool HotkeyManager::handle_hotkey(WPARAM wParam) {
    int id = static_cast<int>(wParam);
    auto it = registered_hotkeys_.find(id);
    if (it == registered_hotkeys_.end()) return false;

    if (callback_) {
        callback_(it->second, true); // key_down = true for WM_HOTKEY
    }

    return true;
}

} // namespace localvoice
