#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// WASAPI Audio Capture — Windows Core Audio implementation
// ─────────────────────────────────────────────────────────────────────────────

#include "audio/audio_capture.h"
#include "audio/ring_buffer.h"

#include <thread>
#include <atomic>
#include <memory>

// Forward declarations for COM interfaces (avoid including heavy headers here)
struct IMMDeviceEnumerator;
struct IMMDevice;
struct IAudioClient;
struct IAudioCaptureClient;

namespace localvoice {

class WasapiCapture : public IAudioCapture {
public:
    WasapiCapture();
    ~WasapiCapture() override;

    std::vector<AudioDeviceInfo> enumerate_devices() override;
    VoidResult start(const std::string& device_id, AudioDataCallback callback) override;
    void stop() override;
    [[nodiscard]] bool is_capturing() const override;
    [[nodiscard]] std::optional<AudioDeviceInfo> current_device() const override;

private:
    void capture_thread_func();
    VoidResult init_com();
    void release_com();

    AudioDataCallback       callback_;
    std::thread             capture_thread_;
    std::atomic<bool>       capturing_{false};
    std::atomic<bool>       should_stop_{false};
    AudioDeviceInfo         current_device_info_;

    // COM interfaces — managed manually for RAII
    IMMDeviceEnumerator*    enumerator_ = nullptr;
    IMMDevice*              device_     = nullptr;
    IAudioClient*           audio_client_ = nullptr;
    IAudioCaptureClient*    capture_client_ = nullptr;
};

} // namespace localvoice
