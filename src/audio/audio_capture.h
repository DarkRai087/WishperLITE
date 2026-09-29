#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// IAudioCapture — interface for audio capture backends
// ─────────────────────────────────────────────────────────────────────────────

#include "core/types.h"

#include <string>
#include <vector>
#include <functional>

namespace localvoice {

/// Describes an audio input device
struct AudioDeviceInfo {
    std::string id;            ///< System device ID
    std::string name;          ///< Human-readable name
    bool        is_default;    ///< True if this is the system default
    uint32_t    sample_rate;   ///< Native sample rate
    uint32_t    channels;      ///< Number of channels
};

/// Callback invoked when audio data is available.
/// The span is valid only for the duration of the callback.
/// Implementations must not allocate or block.
using AudioDataCallback = std::function<void(AudioSpan samples, uint32_t sample_rate, uint32_t channels)>;

/// Abstract interface for audio capture
class IAudioCapture {
public:
    virtual ~IAudioCapture() = default;

    /// Enumerate available audio input devices
    [[nodiscard]] virtual std::vector<AudioDeviceInfo> enumerate_devices() = 0;

    /// Start capturing from the specified device (empty = default)
    virtual VoidResult start(const std::string& device_id, AudioDataCallback callback) = 0;

    /// Stop capturing
    virtual void stop() = 0;

    /// Check if currently capturing
    [[nodiscard]] virtual bool is_capturing() const = 0;

    /// Get the current device info
    [[nodiscard]] virtual std::optional<AudioDeviceInfo> current_device() const = 0;
};

} // namespace localvoice
