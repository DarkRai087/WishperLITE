#include "audio/wasapi_capture.h"
#include "core/logger.h"

#include <Windows.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#include <avrt.h>

#include <string>
#include <format>

#pragma comment(lib, "avrt.lib")

namespace localvoice {

// ─────────────────────────────────────────────────────────────────────────────
// Helper: convert HRESULT to readable string
// ─────────────────────────────────────────────────────────────────────────────
static std::string hr_to_string(HRESULT hr) {
    return std::format("HRESULT 0x{:08X}", static_cast<unsigned>(hr));
}

// ─────────────────────────────────────────────────────────────────────────────
// Helper: wide string to UTF-8
// ─────────────────────────────────────────────────────────────────────────────
static std::string wstring_to_utf8(const std::wstring& wstr) {
    if (wstr.empty()) return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.data(),
        static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
    std::string result(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(),
        static_cast<int>(wstr.size()), result.data(), size, nullptr, nullptr);
    return result;
}

WasapiCapture::WasapiCapture() = default;

WasapiCapture::~WasapiCapture() {
    stop();
    release_com();
}

VoidResult WasapiCapture::init_com() {
    // CoInitializeEx must be called on each thread that uses COM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != S_FALSE && hr != RPC_E_CHANGED_MODE) {
        return std::unexpected(Error{ErrorCode::AudioDeviceOpenFailed,
            "COM initialization failed: " + hr_to_string(hr)});
    }

    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator),
        reinterpret_cast<void**>(&enumerator_));

    if (FAILED(hr) || !enumerator_) {
        return std::unexpected(Error{ErrorCode::AudioDeviceOpenFailed,
            "Failed to create device enumerator: " + hr_to_string(hr)});
    }

    return {};
}

void WasapiCapture::release_com() {
    if (capture_client_) { capture_client_->Release(); capture_client_ = nullptr; }
    if (audio_client_)   { audio_client_->Release();   audio_client_   = nullptr; }
    if (device_)         { device_->Release();         device_         = nullptr; }
    if (enumerator_)     { enumerator_->Release();     enumerator_     = nullptr; }
}

std::vector<AudioDeviceInfo> WasapiCapture::enumerate_devices() {
    std::vector<AudioDeviceInfo> devices;

    auto init_result = init_com();
    if (!init_result) {
        LV_ERROR("audio", "Cannot enumerate: " + init_result.error().message);
        return devices;
    }

    // Get default device ID for comparison
    std::wstring default_id;
    {
        IMMDevice* def_device = nullptr;
        if (SUCCEEDED(enumerator_->GetDefaultAudioEndpoint(eCapture, eConsole, &def_device)) && def_device) {
            LPWSTR id = nullptr;
            if (SUCCEEDED(def_device->GetId(&id)) && id) {
                default_id = id;
                CoTaskMemFree(id);
            }
            def_device->Release();
        }
    }

    // Enumerate all capture devices
    IMMDeviceCollection* collection = nullptr;
    HRESULT hr = enumerator_->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &collection);
    if (FAILED(hr) || !collection) {
        LV_WARN("audio", "No audio capture devices found");
        release_com();
        return devices;
    }

    UINT count = 0;
    collection->GetCount(&count);

    for (UINT i = 0; i < count; ++i) {
        IMMDevice* dev = nullptr;
        if (FAILED(collection->Item(i, &dev)) || !dev) continue;

        AudioDeviceInfo info;

        // Device ID
        LPWSTR id = nullptr;
        if (SUCCEEDED(dev->GetId(&id)) && id) {
            info.id = wstring_to_utf8(id);
            info.is_default = (id == default_id);
            CoTaskMemFree(id);
        }

        // Friendly name
        IPropertyStore* props = nullptr;
        if (SUCCEEDED(dev->OpenPropertyStore(STGM_READ, &props)) && props) {
            PROPVARIANT name;
            PropVariantInit(&name);
            if (SUCCEEDED(props->GetValue(PKEY_Device_FriendlyName, &name))) {
                if (name.vt == VT_LPWSTR && name.pwszVal) {
                    info.name = wstring_to_utf8(name.pwszVal);
                }
            }
            PropVariantClear(&name);
            props->Release();
        }

        // Get format info
        IAudioClient* client = nullptr;
        if (SUCCEEDED(dev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                                    reinterpret_cast<void**>(&client))) && client) {
            WAVEFORMATEX* fmt = nullptr;
            if (SUCCEEDED(client->GetMixFormat(&fmt)) && fmt) {
                info.sample_rate = fmt->nSamplesPerSec;
                info.channels = fmt->nChannels;
                CoTaskMemFree(fmt);
            }
            client->Release();
        }

        devices.push_back(std::move(info));
        dev->Release();
    }

    collection->Release();
    release_com();

    LV_INFO("audio", std::format("Found {} audio capture device(s)", devices.size()));
    return devices;
}

VoidResult WasapiCapture::start(const std::string& device_id, AudioDataCallback callback) {
    if (capturing_.load(std::memory_order_acquire)) {
        return std::unexpected(Error{ErrorCode::AlreadyInitialized,
            "Audio capture already started"});
    }

    callback_ = std::move(callback);
    should_stop_.store(false, std::memory_order_release);

    auto init_result = init_com();
    if (!init_result) return init_result;

    // Get the requested device (or default)
    HRESULT hr;
    if (device_id.empty()) {
        hr = enumerator_->GetDefaultAudioEndpoint(eCapture, eConsole, &device_);
        if (FAILED(hr) || !device_) {
            release_com();
            return std::unexpected(Error{ErrorCode::AudioDeviceNotFound,
                "No default audio capture device: " + hr_to_string(hr)});
        }
        LV_INFO("audio", "Using default audio capture device");
    } else {
        // Convert device_id to wide string for WASAPI
        int wlen = MultiByteToWideChar(CP_UTF8, 0, device_id.c_str(),
            static_cast<int>(device_id.size()), nullptr, 0);
        std::wstring wid(wlen, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, device_id.c_str(),
            static_cast<int>(device_id.size()), wid.data(), wlen);

        hr = enumerator_->GetDevice(wid.c_str(), &device_);
        if (FAILED(hr) || !device_) {
            release_com();
            return std::unexpected(Error{ErrorCode::AudioDeviceNotFound,
                "Device not found: " + device_id});
        }
        LV_INFO("audio", "Using audio device: " + device_id);
    }

    // Activate audio client
    hr = device_->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                           reinterpret_cast<void**>(&audio_client_));
    if (FAILED(hr) || !audio_client_) {
        release_com();
        return std::unexpected(Error{ErrorCode::AudioDeviceOpenFailed,
            "Failed to activate audio client: " + hr_to_string(hr)});
    }

    // Get the mix format
    WAVEFORMATEX* mix_format = nullptr;
    hr = audio_client_->GetMixFormat(&mix_format);
    if (FAILED(hr) || !mix_format) {
        release_com();
        return std::unexpected(Error{ErrorCode::AudioDeviceOpenFailed,
            "Failed to get mix format: " + hr_to_string(hr)});
    }

    current_device_info_.sample_rate = mix_format->nSamplesPerSec;
    current_device_info_.channels = mix_format->nChannels;

    LV_INFO("audio", std::format("Device format: {}Hz, {} ch, {} bits",
        mix_format->nSamplesPerSec, mix_format->nChannels, mix_format->wBitsPerSample));

    // Initialize audio client in shared mode with event-driven buffering
    // Buffer duration: 20ms — low latency but not so low as to cause glitches
    REFERENCE_TIME buffer_duration = 200000; // 20ms in 100ns units
    hr = audio_client_->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
        buffer_duration,
        0,
        mix_format,
        nullptr);

    if (FAILED(hr)) {
        // Fallback: try without event callback
        hr = audio_client_->Initialize(
            AUDCLNT_SHAREMODE_SHARED,
            0,
            buffer_duration,
            0,
            mix_format,
            nullptr);
    }

    CoTaskMemFree(mix_format);

    if (FAILED(hr)) {
        release_com();
        return std::unexpected(Error{ErrorCode::AudioCaptureStartFailed,
            "Failed to initialize audio client: " + hr_to_string(hr)});
    }

    // Get capture client
    hr = audio_client_->GetService(__uuidof(IAudioCaptureClient),
                                   reinterpret_cast<void**>(&capture_client_));
    if (FAILED(hr) || !capture_client_) {
        release_com();
        return std::unexpected(Error{ErrorCode::AudioCaptureStartFailed,
            "Failed to get capture client: " + hr_to_string(hr)});
    }

    // Start the audio stream
    hr = audio_client_->Start();
    if (FAILED(hr)) {
        release_com();
        return std::unexpected(Error{ErrorCode::AudioCaptureStartFailed,
            "Failed to start audio stream: " + hr_to_string(hr)});
    }

    capturing_.store(true, std::memory_order_release);

    // Launch capture thread
    capture_thread_ = std::thread(&WasapiCapture::capture_thread_func, this);

    LV_INFO("audio", "Audio capture started");
    return {};
}

void WasapiCapture::stop() {
    if (!capturing_.load(std::memory_order_acquire)) return;

    should_stop_.store(true, std::memory_order_release);

    if (capture_thread_.joinable()) {
        capture_thread_.join();
    }

    if (audio_client_) {
        audio_client_->Stop();
    }

    capturing_.store(false, std::memory_order_release);
    release_com();

    LV_INFO("audio", "Audio capture stopped");
}

bool WasapiCapture::is_capturing() const {
    return capturing_.load(std::memory_order_acquire);
}

std::optional<AudioDeviceInfo> WasapiCapture::current_device() const {
    if (!capturing_.load(std::memory_order_acquire)) return std::nullopt;
    return current_device_info_;
}

void WasapiCapture::capture_thread_func() {
    // Register with MMCSS for audio thread scheduling priority
    DWORD task_index = 0;
    HANDLE task = AvSetMmThreadCharacteristicsW(L"Audio", &task_index);
    if (!task) {
        LV_WARN("audio", "Failed to set MMCSS thread characteristics");
    }

    LV_DEBUG("audio", "Capture thread started");

    while (!should_stop_.load(std::memory_order_acquire)) {
        // Sleep briefly to avoid spinning — 5ms is ~half our buffer period
        Sleep(5);

        if (!capture_client_) break;

        UINT32 packet_length = 0;
        HRESULT hr = capture_client_->GetNextPacketSize(&packet_length);
        if (FAILED(hr)) {
            LV_ERROR("audio", "GetNextPacketSize failed: " + hr_to_string(hr));
            break;
        }

        while (packet_length > 0) {
            BYTE*  data = nullptr;
            UINT32 num_frames = 0;
            DWORD  flags = 0;

            hr = capture_client_->GetBuffer(&data, &num_frames, &flags, nullptr, nullptr);
            if (FAILED(hr)) {
                LV_ERROR("audio", "GetBuffer failed: " + hr_to_string(hr));
                break;
            }

            if (num_frames > 0 && callback_) {
                if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                    // Silent buffer — pass zeros
                    // We still call the callback so the pipeline knows time is passing
                    std::vector<AudioSample> silence(num_frames, 0.0f);
                    callback_(AudioSpan{silence},
                              current_device_info_.sample_rate,
                              current_device_info_.channels);
                } else {
                    // Real audio data — interpret as float32 (WASAPI shared mode typically
                    // uses IEEE float). The data pointer is valid only until ReleaseBuffer.
                    auto* float_data = reinterpret_cast<const AudioSample*>(data);
                    size_t total_samples = static_cast<size_t>(num_frames) * current_device_info_.channels;
                    callback_(AudioSpan{float_data, total_samples},
                              current_device_info_.sample_rate,
                              current_device_info_.channels);
                }
            }

            capture_client_->ReleaseBuffer(num_frames);

            hr = capture_client_->GetNextPacketSize(&packet_length);
            if (FAILED(hr)) break;
        }
    }

    if (task) {
        AvRevertMmThreadCharacteristics(task);
    }

    LV_DEBUG("audio", "Capture thread exiting");
}

} // namespace localvoice
