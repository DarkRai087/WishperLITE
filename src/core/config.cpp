#include "core/config.h"
#include "core/logger.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <filesystem>
#include <sstream>

#ifdef _WIN32
#include <ShlObj.h>   // SHGetFolderPathW
#include <Windows.h>
#endif

namespace localvoice {

using json = nlohmann::json;
namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────────
// Path helpers
// ─────────────────────────────────────────────────────────────────────────────

static std::string get_appdata_dir() {
#ifdef _WIN32
    wchar_t path[MAX_PATH]{};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, path))) {
        auto p = fs::path(path) / "LocalVoice";
        fs::create_directories(p);
        return p.string();
    }
#endif
    // Fallback: current directory
    return (fs::current_path() / "LocalVoice").string();
}

std::string get_default_config_path() {
    return (fs::path(get_appdata_dir()) / "config.json").string();
}

std::string get_default_models_path() {
    auto p = fs::path(get_appdata_dir()) / "models";
    fs::create_directories(p);
    return p.string();
}

std::string get_default_log_path() {
    auto p = fs::path(get_appdata_dir()) / "logs";
    fs::create_directories(p);
    return (p / "localvoice.log").string();
}

// ─────────────────────────────────────────────────────────────────────────────
// JSON serialization helpers
// ─────────────────────────────────────────────────────────────────────────────

static json audio_to_json(const AudioConfig& c) {
    return json{
        {"device_id",   c.device_id},
        {"sample_rate", c.sample_rate},
        {"channels",    c.channels},
        {"buffer_ms",   c.buffer_ms},
    };
}

static AudioConfig audio_from_json(const json& j) {
    AudioConfig c;
    if (j.contains("device_id"))   c.device_id   = j["device_id"].get<std::string>();
    if (j.contains("sample_rate")) c.sample_rate  = j["sample_rate"].get<uint32_t>();
    if (j.contains("channels"))    c.channels     = j["channels"].get<uint32_t>();
    if (j.contains("buffer_ms"))   c.buffer_ms    = j["buffer_ms"].get<uint32_t>();
    return c;
}

static json vad_to_json(const VadConfig& c) {
    return json{
        {"threshold",               c.threshold},
        {"min_speech_duration_ms",  c.min_speech_duration_ms},
        {"min_silence_duration_ms", c.min_silence_duration_ms},
        {"max_speech_duration_s",   c.max_speech_duration_s},
        {"speech_pad_ms",           c.speech_pad_ms},
        {"samples_overlap",         c.samples_overlap},
    };
}

static VadConfig vad_from_json(const json& j) {
    VadConfig c;
    if (j.contains("threshold"))               c.threshold               = j["threshold"].get<float>();
    if (j.contains("min_speech_duration_ms"))   c.min_speech_duration_ms  = j["min_speech_duration_ms"].get<int>();
    if (j.contains("min_silence_duration_ms"))  c.min_silence_duration_ms = j["min_silence_duration_ms"].get<int>();
    if (j.contains("max_speech_duration_s"))    c.max_speech_duration_s   = j["max_speech_duration_s"].get<float>();
    if (j.contains("speech_pad_ms"))            c.speech_pad_ms           = j["speech_pad_ms"].get<int>();
    if (j.contains("samples_overlap"))          c.samples_overlap         = j["samples_overlap"].get<float>();
    return c;
}

static json stt_to_json(const SttConfig& c) {
    return json{
        {"model_path",        c.model_path},
        {"vad_model_path",    c.vad_model_path},
        {"language",          c.language},
        {"use_gpu",           c.use_gpu},
        {"gpu_device",        c.gpu_device},
        {"flash_attn",        c.flash_attn},
        {"keep_model_loaded", c.keep_model_loaded},
        {"n_threads",         c.n_threads},
    };
}

static SttConfig stt_from_json(const json& j) {
    SttConfig c;
    if (j.contains("model_path"))        c.model_path        = j["model_path"].get<std::string>();
    if (j.contains("vad_model_path"))    c.vad_model_path    = j["vad_model_path"].get<std::string>();
    if (j.contains("language"))          c.language           = j["language"].get<std::string>();
    if (j.contains("use_gpu"))           c.use_gpu            = j["use_gpu"].get<bool>();
    if (j.contains("gpu_device"))        c.gpu_device         = j["gpu_device"].get<int>();
    if (j.contains("flash_attn"))        c.flash_attn         = j["flash_attn"].get<bool>();
    if (j.contains("keep_model_loaded")) c.keep_model_loaded  = j["keep_model_loaded"].get<bool>();
    if (j.contains("n_threads"))         c.n_threads          = j["n_threads"].get<int>();
    return c;
}

static json hotkey_to_json(const HotkeyConfig& c) {
    return json{
        {"push_to_talk_vk",   c.push_to_talk_vk},
        {"push_to_talk_mods", c.push_to_talk_mods},
        {"toggle_vk",         c.toggle_vk},
        {"toggle_mods",       c.toggle_mods},
    };
}

static HotkeyConfig hotkey_from_json(const json& j) {
    HotkeyConfig c;
    if (j.contains("push_to_talk_vk"))   c.push_to_talk_vk   = j["push_to_talk_vk"].get<uint32_t>();
    if (j.contains("push_to_talk_mods")) c.push_to_talk_mods  = j["push_to_talk_mods"].get<uint32_t>();
    if (j.contains("toggle_vk"))         c.toggle_vk          = j["toggle_vk"].get<uint32_t>();
    if (j.contains("toggle_mods"))       c.toggle_mods        = j["toggle_mods"].get<uint32_t>();
    return c;
}

static json ui_to_json(const UiConfig& c) {
    return json{
        {"show_window_on_start", c.show_window_on_start},
        {"start_minimized",      c.start_minimized},
        {"window_width",         c.window_width},
        {"window_height",        c.window_height},
    };
}

static UiConfig ui_from_json(const json& j) {
    UiConfig c;
    if (j.contains("show_window_on_start")) c.show_window_on_start = j["show_window_on_start"].get<bool>();
    if (j.contains("start_minimized"))      c.start_minimized      = j["start_minimized"].get<bool>();
    if (j.contains("window_width"))         c.window_width         = j["window_width"].get<int>();
    if (j.contains("window_height"))        c.window_height        = j["window_height"].get<int>();
    return c;
}

static json log_to_json(const LogConfig& c) {
    return json{
        {"log_file_path",  c.log_file_path},
        {"min_level",      c.min_level},
        {"log_to_console", c.log_to_console},
        {"log_to_file",    c.log_to_file},
    };
}

static LogConfig log_from_json(const json& j) {
    LogConfig c;
    if (j.contains("log_file_path"))  c.log_file_path  = j["log_file_path"].get<std::string>();
    if (j.contains("min_level"))      c.min_level       = j["min_level"].get<std::string>();
    if (j.contains("log_to_console")) c.log_to_console  = j["log_to_console"].get<bool>();
    if (j.contains("log_to_file"))    c.log_to_file     = j["log_to_file"].get<bool>();
    return c;
}

static json game_mode_to_json(const GameModeConfig& c) {
    return json{
        {"enabled",                c.enabled},
        {"max_inference_threads",  c.max_inference_threads},
        {"inference_cooldown_s",   c.inference_cooldown_s},
        {"prefer_cpu",             c.prefer_cpu},
    };
}

static GameModeConfig game_mode_from_json(const json& j) {
    GameModeConfig c;
    if (j.contains("enabled"))                c.enabled               = j["enabled"].get<bool>();
    if (j.contains("max_inference_threads"))  c.max_inference_threads = j["max_inference_threads"].get<int>();
    if (j.contains("inference_cooldown_s"))   c.inference_cooldown_s  = j["inference_cooldown_s"].get<float>();
    if (j.contains("prefer_cpu"))             c.prefer_cpu            = j["prefer_cpu"].get<bool>();
    return c;
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

AppConfig load_config(const std::string& path) {
    AppConfig config;

    if (!fs::exists(path)) {
        LV_INFO("config", "Config file not found, using defaults: " + path);
        return config;
    }

    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            LV_WARN("config", "Cannot open config file: " + path);
            return config;
        }

        json j = json::parse(file);

        if (j.contains("audio"))     config.audio     = audio_from_json(j["audio"]);
        if (j.contains("vad"))       config.vad       = vad_from_json(j["vad"]);
        if (j.contains("stt"))       config.stt       = stt_from_json(j["stt"]);
        if (j.contains("hotkeys"))   config.hotkeys   = hotkey_from_json(j["hotkeys"]);
        if (j.contains("ui"))        config.ui        = ui_from_json(j["ui"]);
        if (j.contains("logging"))   config.logging   = log_from_json(j["logging"]);
        if (j.contains("game_mode")) config.game_mode = game_mode_from_json(j["game_mode"]);

        if (j.contains("mode")) {
            auto mode_str = j["mode"].get<std::string>();
            if (mode_str == "continuous") config.mode = TranscriptionMode::Continuous;
            else config.mode = TranscriptionMode::PushToTalk;
        }

        LV_INFO("config", "Configuration loaded from: " + path);
    } catch (const json::parse_error& e) {
        LV_ERROR("config", std::string("Config parse error: ") + e.what());
    } catch (const std::exception& e) {
        LV_ERROR("config", std::string("Config load error: ") + e.what());
    }

    return config;
}

VoidResult save_config(const AppConfig& config, const std::string& path) {
    try {
        auto parent = fs::path(path).parent_path();
        if (!parent.empty()) {
            fs::create_directories(parent);
        }

        json j;
        j["audio"]     = audio_to_json(config.audio);
        j["vad"]       = vad_to_json(config.vad);
        j["stt"]       = stt_to_json(config.stt);
        j["hotkeys"]   = hotkey_to_json(config.hotkeys);
        j["ui"]        = ui_to_json(config.ui);
        j["logging"]   = log_to_json(config.logging);
        j["game_mode"] = game_mode_to_json(config.game_mode);
        j["mode"]      = (config.mode == TranscriptionMode::Continuous) ? "continuous" : "push_to_talk";

        std::ofstream file(path);
        if (!file.is_open()) {
            return std::unexpected(Error{ErrorCode::ConfigParseError,
                "Cannot open config file for writing: " + path});
        }

        file << j.dump(2);
        LV_INFO("config", "Configuration saved to: " + path);
        return {};
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::ConfigParseError,
            std::string("Config save error: ") + e.what()});
    }
}

} // namespace localvoice
