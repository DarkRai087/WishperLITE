#include "vad/speech_segmenter.h"
#include "core/logger.h"

#include <format>

namespace localvoice {

SpeechSegmenter::SpeechSegmenter(const VadConfig& config)
    : config_(config) {
    // Pre-calculate sample counts from ms durations
    min_speech_samples_  = static_cast<size_t>(config.min_speech_duration_ms * kWhisperSampleRate / 1000);
    min_silence_samples_ = static_cast<size_t>(config.min_silence_duration_ms * kWhisperSampleRate / 1000);
    max_speech_samples_  = static_cast<size_t>(config.max_speech_duration_s * kWhisperSampleRate);
    speech_pad_samples_  = static_cast<size_t>(config.speech_pad_ms * kWhisperSampleRate / 1000);

    // Pre-allocate the accumulator for the maximum segment size + padding
    accumulator_.reserve(max_speech_samples_ + speech_pad_samples_ * 2);
}

void SpeechSegmenter::process(AudioSpan samples, IVAD& vad, SpeechSegmentCallback on_segment) {
    auto result = vad.process(samples);

    switch (state_) {
    case SegmenterState::Silence:
        if (result.is_speech) {
            transition_to(SegmenterState::Speaking);
            // Add pre-speech padding (we don't have history here in the MVP —
            // in a future version, we could maintain a small lookback buffer)
            accumulator_.insert(accumulator_.end(), samples.begin(), samples.end());
            speech_samples_ = samples.size();
        }
        break;

    case SegmenterState::Speaking:
        accumulator_.insert(accumulator_.end(), samples.begin(), samples.end());
        speech_samples_ += samples.size();

        if (!result.is_speech) {
            transition_to(SegmenterState::PostSpeech);
            silence_samples_ = samples.size();
        } else if (speech_samples_ >= max_speech_samples_) {
            // Force split — segment is too long
            LV_DEBUG("segmenter", std::format("Forcing segment split at {} samples",
                speech_samples_));
            finalize_segment(on_segment);
        }
        break;

    case SegmenterState::PostSpeech:
        accumulator_.insert(accumulator_.end(), samples.begin(), samples.end());

        if (result.is_speech) {
            // Speech resumed — go back to speaking
            transition_to(SegmenterState::Speaking);
            silence_samples_ = 0;
        } else {
            silence_samples_ += samples.size();
            if (silence_samples_ >= min_silence_samples_) {
                // Silence confirmed — finalize the segment
                if (speech_samples_ >= min_speech_samples_) {
                    finalize_segment(on_segment);
                } else {
                    // Too short — discard
                    LV_DEBUG("segmenter", std::format(
                        "Discarding short segment: {} samples (min: {})",
                        speech_samples_, min_speech_samples_));
                    reset();
                }
            }
        }
        break;
    }
}

void SpeechSegmenter::force_finalize(SpeechSegmentCallback on_segment) {
    if (state_ != SegmenterState::Silence && !accumulator_.empty()) {
        finalize_segment(on_segment);
    }
}

void SpeechSegmenter::reset() {
    state_ = SegmenterState::Silence;
    accumulator_.clear();
    silence_samples_ = 0;
    speech_samples_ = 0;
}

void SpeechSegmenter::transition_to(SegmenterState new_state) {
    if (state_ != new_state) {
        LV_TRACE("segmenter", std::format("State: {} -> {}",
            static_cast<int>(state_), static_cast<int>(new_state)));
        state_ = new_state;
    }
}

void SpeechSegmenter::finalize_segment(SpeechSegmentCallback& on_segment) {
    if (on_segment && !accumulator_.empty()) {
        LV_DEBUG("segmenter", std::format("Segment finalized: {} samples ({:.1f}s)",
            accumulator_.size(),
            static_cast<float>(accumulator_.size()) / kWhisperSampleRate));

        // Move the accumulated audio to the callback — zero-copy transfer
        on_segment(std::move(accumulator_));
    }

    // Reset for next segment (accumulator_ is in valid-but-unspecified state after move)
    accumulator_ = std::vector<AudioSample>();
    accumulator_.reserve(max_speech_samples_ + speech_pad_samples_ * 2);
    silence_samples_ = 0;
    speech_samples_ = 0;
    state_ = SegmenterState::Silence;
}

} // namespace localvoice
