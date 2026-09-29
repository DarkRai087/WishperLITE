#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// Common types used throughout LocalVoice
// ─────────────────────────────────────────────────────────────────────────────

#include <cstdint>
#include <chrono>
#include <string>
#include <string_view>
#include <span>
#include <functional>
#include <optional>
#include <expected>
#include <memory>

namespace localvoice {

// ─────────────────────────────────────────────────────────────────────────────
// Time types
// ─────────────────────────────────────────────────────────────────────────────
using Clock     = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using Duration  = Clock::duration;
using Milliseconds = std::chrono::milliseconds;
using Microseconds = std::chrono::microseconds;

// ─────────────────────────────────────────────────────────────────────────────
// Audio types
// ─────────────────────────────────────────────────────────────────────────────

/// Audio sample in 32-bit float format (range: -1.0 to 1.0)
using AudioSample = float;

/// A non-owning view into a buffer of audio samples
using AudioSpan = std::span<const AudioSample>;

/// A mutable non-owning view into a buffer of audio samples
using MutableAudioSpan = std::span<AudioSample>;

/// Standard sample rate for Whisper inference
inline constexpr uint32_t kWhisperSampleRate = 16000;

/// Maximum speech segment duration in seconds
inline constexpr float kMaxSpeechDurationSec = 30.0f;

/// Maximum speech segment in samples at 16 kHz
inline constexpr size_t kMaxSpeechSamples =
    static_cast<size_t>(kWhisperSampleRate * kMaxSpeechDurationSec);

// ─────────────────────────────────────────────────────────────────────────────
// Error handling
// ─────────────────────────────────────────────────────────────────────────────

/// Error codes for LocalVoice operations
enum class ErrorCode : uint32_t {
    Ok = 0,

    // Audio errors (100-199)
    AudioDeviceNotFound      = 100,
    AudioDeviceOpenFailed    = 101,
    AudioDeviceDisconnected  = 102,
    AudioCaptureStartFailed  = 103,
    AudioCaptureError        = 104,
    AudioResampleFailed      = 105,
    AudioBufferOverflow      = 106,

    // Model errors (200-299)
    ModelNotFound            = 200,
    ModelLoadFailed          = 201,
    ModelCorrupted           = 202,
    ModelIncompatible        = 203,

    // Inference errors (300-399)
    InferenceFailed          = 300,
    InferenceCancelled       = 301,
    InferenceTimeout         = 302,
    CudaUnavailable          = 303,
    CudaOutOfMemory          = 304,

    // Config errors (400-499)
    ConfigNotFound           = 400,
    ConfigParseError         = 401,
    ConfigValidationError    = 402,

    // Platform errors (500-599)
    HotkeyRegistrationFailed = 500,
    TrayIconFailed           = 501,
    WindowCreationFailed     = 502,

    // General errors (900-999)
    NotInitialized           = 900,
    AlreadyInitialized       = 901,
    InvalidArgument          = 902,
    InternalError            = 999,
};

/// Human-readable name for an error code
constexpr std::string_view error_code_name(ErrorCode code) {
    switch (code) {
        case ErrorCode::Ok:                       return "Ok";
        case ErrorCode::AudioDeviceNotFound:      return "AudioDeviceNotFound";
        case ErrorCode::AudioDeviceOpenFailed:    return "AudioDeviceOpenFailed";
        case ErrorCode::AudioDeviceDisconnected:  return "AudioDeviceDisconnected";
        case ErrorCode::AudioCaptureStartFailed:  return "AudioCaptureStartFailed";
        case ErrorCode::AudioCaptureError:        return "AudioCaptureError";
        case ErrorCode::AudioResampleFailed:      return "AudioResampleFailed";
        case ErrorCode::AudioBufferOverflow:      return "AudioBufferOverflow";
        case ErrorCode::ModelNotFound:            return "ModelNotFound";
        case ErrorCode::ModelLoadFailed:          return "ModelLoadFailed";
        case ErrorCode::ModelCorrupted:           return "ModelCorrupted";
        case ErrorCode::ModelIncompatible:        return "ModelIncompatible";
        case ErrorCode::InferenceFailed:          return "InferenceFailed";
        case ErrorCode::InferenceCancelled:       return "InferenceCancelled";
        case ErrorCode::InferenceTimeout:         return "InferenceTimeout";
        case ErrorCode::CudaUnavailable:          return "CudaUnavailable";
        case ErrorCode::CudaOutOfMemory:          return "CudaOutOfMemory";
        case ErrorCode::ConfigNotFound:           return "ConfigNotFound";
        case ErrorCode::ConfigParseError:         return "ConfigParseError";
        case ErrorCode::ConfigValidationError:    return "ConfigValidationError";
        case ErrorCode::HotkeyRegistrationFailed: return "HotkeyRegistrationFailed";
        case ErrorCode::TrayIconFailed:           return "TrayIconFailed";
        case ErrorCode::WindowCreationFailed:     return "WindowCreationFailed";
        case ErrorCode::NotInitialized:           return "NotInitialized";
        case ErrorCode::AlreadyInitialized:       return "AlreadyInitialized";
        case ErrorCode::InvalidArgument:          return "InvalidArgument";
        case ErrorCode::InternalError:            return "InternalError";
        default:                                  return "Unknown";
    }
}

/// Error type carrying a code and a message
struct Error {
    ErrorCode code;
    std::string message;

    Error(ErrorCode c, std::string msg)
        : code(c), message(std::move(msg)) {}
};

/// Result type: either a value T or an Error
template<typename T>
using Result = std::expected<T, Error>;

/// Result type for operations that return nothing on success
using VoidResult = std::expected<void, Error>;

// ─────────────────────────────────────────────────────────────────────────────
// Transcription types
// ─────────────────────────────────────────────────────────────────────────────

/// A single transcription segment with timing information
struct TranscriptSegment {
    std::string text;
    int64_t start_ms = 0;     ///< Start time in milliseconds
    int64_t end_ms   = 0;     ///< End time in milliseconds
    float   confidence = 0.0f; ///< Confidence score (0.0 to 1.0), if available
};

/// Complete transcription result from a single inference call
struct TranscriptResult {
    std::vector<TranscriptSegment> segments;
    Duration inference_duration{};  ///< How long inference took
    size_t   audio_samples = 0;    ///< Number of audio samples processed
    bool     was_cancelled = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// Application state
// ─────────────────────────────────────────────────────────────────────────────

/// High-level application state
enum class AppState : uint8_t {
    Uninitialized,
    Initializing,
    Idle,             ///< Ready but not transcribing
    Listening,        ///< VAD active, waiting for speech
    Transcribing,     ///< Whisper inference running
    Error,
    ShuttingDown,
};

/// Transcription mode
enum class TranscriptionMode : uint8_t {
    Continuous,       ///< Always listening via VAD
    PushToTalk,       ///< Only when hotkey is held
};

} // namespace localvoice
