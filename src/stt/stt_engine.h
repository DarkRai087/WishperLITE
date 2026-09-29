#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// ISTTEngine — Speech-to-Text engine interface
// ─────────────────────────────────────────────────────────────────────────────

#include "core/types.h"

#include <vector>
#include <atomic>
#include <string>

namespace localvoice {

/// Configuration for an STT inference call
struct InferenceParams {
    std::string language = "en";
    int         n_threads = 4;
    bool        translate = false;   ///< Future: translate to English
    bool        timestamps = true;   ///< Include timing info
};

/// Abstract STT engine interface
class ISTTEngine {
public:
    virtual ~ISTTEngine() = default;

    /// Load a model from disk
    virtual VoidResult load_model(const std::string& model_path, bool use_gpu, int gpu_device) = 0;

    /// Unload the current model
    virtual void unload_model() = 0;

    /// Check if a model is loaded
    [[nodiscard]] virtual bool is_model_loaded() const = 0;

    /// Transcribe audio. The audio must be 16 kHz mono float32.
    /// This is a blocking call that should be invoked on the inference thread.
    virtual Result<TranscriptResult> transcribe(
        AudioSpan audio,
        const InferenceParams& params,
        std::atomic<bool>& cancel_flag) = 0;

    /// Get the name of this engine (for logging/diagnostics)
    [[nodiscard]] virtual std::string engine_name() const = 0;

    /// Get estimated VRAM usage in bytes (0 if CPU-only)
    [[nodiscard]] virtual size_t estimated_vram_bytes() const = 0;

    /// Get estimated RAM usage in bytes
    [[nodiscard]] virtual size_t estimated_ram_bytes() const = 0;
};

} // namespace localvoice
