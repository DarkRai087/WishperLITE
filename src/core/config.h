#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// Configuration management — load, save, validate JSON config
// ─────────────────────────────────────────────────────────────────────────────

#include "core/types.h"

#include <string>
#include <cstdint>

namespace localvoice {

/// Audio configuration
struct AudioConfig {
    std::string device_id;             ///< Empty = default device
    uint32_t    sample_rate   = 16000; ///< Target sample rate
    uint32_t    channels      = 1;     ///< Mono
    uint32_t    buffer_ms     = 30;    ///< Audio buffer size in ms
};

/// VAD configuration — maps to whisper_vad_params
struct VadConfig {
    float    threshold              = 0.5f;   ///< Speech probability threshold
    int      min_speech_duration_ms = 250;    ///< Min speech segment duration
    int      min_silence_duration_ms = 300;   ///< Min silence to end speech
    float    max_speech_duration_s  = 30.0f;  ///< Force segment split
    int      speech_pad_ms         = 200;     ///< Padding before/after speech
    float    samples_overlap       = 0.1f;    ///< Overlap between segments
};

/// STT engine configuration
struct SttConfig {
    std::string model_path;               ///< Path to whisper model file
    std::string vad_model_path;           ///< Path to Silero VAD model file
    std::string language       = "en";    ///< Language code
    bool        use_gpu        = true;    ///< Use CUDA if available
    int         gpu_device     = 0;       ///< CUDA device index
    bool        flash_attn     = false;   ///< Flash attention (if supported)
    bool        keep_model_loaded = true; ///< Keep model in memory when idle
    int         n_threads      = 4;       ///< CPU threads for inference
};

/// Hotkey configuration
struct HotkeyConfig {
    uint32_t push_to_talk_vk   = 0x77;   ///< Virtual key code (F8)
    uint32_t push_to_talk_mods = 0;       ///< Modifier keys (0 = none)
    uint32_t toggle_vk         = 0x76;    ///< Toggle transcription (F7)
    uint32_t toggle_mods       = 0;       ///< Modifier keys for toggle
};

/// UI configuration
struct UiConfig {
    bool show_window_on_start = false;    ///< Show transcript window on startup
    bool start_minimized      = true;     ///< Start in system tray
    int  window_width         = 400;
    int  window_height        = 300;
};

/// Logging configuration
struct LogConfig {
    std::string log_file_path;
    std::string min_level = "info";       ///< trace/debug/info/warn/error/fatal
    bool        log_to_console = true;
    bool        log_to_file    = false;
};

/// Game mode configuration — reduced resource usage
struct GameModeConfig {
    bool    enabled            = false;
    int     max_inference_threads = 2;    ///< Reduce thread count
    float   inference_cooldown_s  = 2.0f; ///< Min seconds between inferences
    bool    prefer_cpu         = false;   ///< Force CPU inference in game mode
};

/// Top-level application configuration
struct AppConfig {
    AudioConfig     audio;
    VadConfig       vad;
    SttConfig       stt;
    HotkeyConfig    hotkeys;
    UiConfig        ui;
    LogConfig       logging;
    GameModeConfig  game_mode;
    TranscriptionMode mode = TranscriptionMode::PushToTalk;
};

/// Load configuration from a JSON file. Returns default config on failure.
AppConfig load_config(const std::string& path);

/// Save configuration to a JSON file.
VoidResult save_config(const AppConfig& config, const std::string& path);

/// Get the default configuration file path (%APPDATA%/LocalVoice/config.json)
std::string get_default_config_path();

/// Get the default models directory path (%APPDATA%/LocalVoice/models/)
std::string get_default_models_path();

/// Get the default log file path (%APPDATA%/LocalVoice/logs/localvoice.log)
std::string get_default_log_path();

} // namespace localvoice
