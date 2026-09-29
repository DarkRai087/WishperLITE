#include "audio/resampler.h"

#include <algorithm>
#include <cmath>

namespace localvoice {

void AudioResampler::configure(uint32_t input_sample_rate, uint32_t input_channels) {
    input_rate_     = input_sample_rate;
    input_channels_ = input_channels;
    ratio_          = static_cast<double>(kWhisperSampleRate) / static_cast<double>(input_rate_);
    position_       = 0.0;

    // Pre-allocate for typical batch sizes
    // 20ms of audio at input rate, converted to mono, then resampled to 16kHz
    size_t typical_input_frames = (input_rate_ * 20) / 1000;
    size_t typical_output = static_cast<size_t>(typical_input_frames * ratio_) + 2;
    output_.reserve(typical_output * 4); // extra headroom
    mono_.reserve(typical_input_frames * 4);
}

void AudioResampler::reset() {
    position_ = 0.0;
    output_.clear();
    mono_.clear();
}

AudioSpan AudioResampler::resample(AudioSpan input) {
    output_.clear();

    if (input.empty() || input_rate_ == 0 || input_channels_ == 0) {
        return AudioSpan{output_};
    }

    const size_t total_samples = input.size();
    const size_t num_frames = total_samples / input_channels_;

    // Step 1: Convert to mono by averaging channels
    mono_.resize(num_frames);
    if (input_channels_ == 1) {
        // Already mono — direct copy
        std::copy(input.begin(), input.end(), mono_.begin());
    } else {
        for (size_t f = 0; f < num_frames; ++f) {
            float sum = 0.0f;
            for (uint32_t ch = 0; ch < input_channels_; ++ch) {
                sum += input[f * input_channels_ + ch];
            }
            mono_[f] = sum / static_cast<float>(input_channels_);
        }
    }

    // Step 2: Resample to 16 kHz using linear interpolation
    if (input_rate_ == kWhisperSampleRate) {
        // No resampling needed
        output_.assign(mono_.begin(), mono_.end());
        return AudioSpan{output_};
    }

    // Estimate output size
    size_t estimated_output = static_cast<size_t>(num_frames * ratio_) + 2;
    output_.reserve(estimated_output);

    while (position_ < static_cast<double>(num_frames) - 1.0) {
        size_t idx = static_cast<size_t>(position_);
        double frac = position_ - static_cast<double>(idx);

        // Linear interpolation between adjacent samples
        float sample;
        if (idx + 1 < num_frames) {
            sample = static_cast<float>(
                mono_[idx] * (1.0 - frac) + mono_[idx + 1] * frac);
        } else {
            sample = mono_[idx];
        }

        output_.push_back(sample);
        position_ += 1.0 / ratio_;
    }

    // Adjust position for continuity across calls
    position_ -= static_cast<double>(num_frames);
    if (position_ < 0.0) position_ = 0.0;

    return AudioSpan{output_};
}

} // namespace localvoice
