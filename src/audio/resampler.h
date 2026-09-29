#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// Audio Resampler — convert from device sample rate to 16 kHz mono
// ─────────────────────────────────────────────────────────────────────────────

#include "core/types.h"
#include <vector>

namespace localvoice {

/// Simple linear interpolation resampler
/// Converts multi-channel audio at any sample rate to 16 kHz mono float32.
/// Uses linear interpolation — sufficient for speech; not HiFi.
class AudioResampler {
public:
    /// Configure the resampler for a given input format
    void configure(uint32_t input_sample_rate, uint32_t input_channels);

    /// Resample input audio to 16 kHz mono.
    /// Output is appended to the output buffer (which is reused).
    /// Returns a span of the resampled output.
    AudioSpan resample(AudioSpan input);

    /// Get the output buffer directly (for zero-copy access)
    [[nodiscard]] const std::vector<AudioSample>& output_buffer() const { return output_; }

    /// Reset internal state
    void reset();

private:
    uint32_t input_rate_     = 0;
    uint32_t input_channels_ = 0;
    double   ratio_          = 1.0;  ///< output_rate / input_rate
    double   position_       = 0.0;  ///< Fractional sample position

    // Pre-allocated output buffer — reused across calls
    std::vector<AudioSample> output_;

    // Pre-allocated mono conversion buffer
    std::vector<AudioSample> mono_;
};

} // namespace localvoice
