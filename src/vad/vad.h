#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// IVAD — Voice Activity Detection interface
// ─────────────────────────────────────────────────────────────────────────────

#include "core/types.h"

namespace localvoice {

/// VAD result for a chunk of audio
struct VadResult {
    bool  is_speech = false;   ///< True if speech was detected
    float probability = 0.0f;  ///< Speech probability (0.0 to 1.0)
};

/// Abstract VAD interface
class IVAD {
public:
    virtual ~IVAD() = default;

    /// Initialize the VAD engine
    virtual VoidResult init(const std::string& model_path) = 0;

    /// Process a chunk of 16 kHz mono float32 audio
    virtual VadResult process(AudioSpan samples) = 0;

    /// Reset internal state (e.g., between segments)
    virtual void reset() = 0;

    /// Check if the VAD is initialized and ready
    [[nodiscard]] virtual bool is_ready() const = 0;
};

} // namespace localvoice
