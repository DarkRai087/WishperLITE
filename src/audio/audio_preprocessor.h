#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// Audio preprocessor — normalization, DC offset removal
// ─────────────────────────────────────────────────────────────────────────────

#include "core/types.h"
#include <vector>

namespace localvoice {

class AudioPreprocessor {
public:
    /// Process audio in-place: remove DC offset, normalize amplitude
    void process(MutableAudioSpan samples);

    /// Reset filter state
    void reset();

private:
    // DC offset removal via single-pole high-pass filter
    float dc_prev_input_  = 0.0f;
    float dc_prev_output_ = 0.0f;
    static constexpr float kDcAlpha = 0.995f; // High-pass cutoff ~8 Hz at 16 kHz
};

} // namespace localvoice
