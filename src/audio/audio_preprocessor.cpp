#include "audio/audio_preprocessor.h"

#include <algorithm>
#include <cmath>

namespace localvoice {

void AudioPreprocessor::process(MutableAudioSpan samples) {
    // DC offset removal using a single-pole high-pass IIR filter:
    //   y[n] = alpha * (y[n-1] + x[n] - x[n-1])
    // This removes sub-8Hz content including DC bias from cheap microphones.
    for (auto& sample : samples) {
        float input = sample;
        dc_prev_output_ = kDcAlpha * (dc_prev_output_ + input - dc_prev_input_);
        dc_prev_input_ = input;
        sample = dc_prev_output_;
    }

    // Soft clamp to [-1.0, 1.0] — protects against microphone clipping artifacts
    for (auto& sample : samples) {
        sample = std::clamp(sample, -1.0f, 1.0f);
    }
}

void AudioPreprocessor::reset() {
    dc_prev_input_ = 0.0f;
    dc_prev_output_ = 0.0f;
}

} // namespace localvoice
