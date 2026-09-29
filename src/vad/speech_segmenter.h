#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// Speech Segmenter — state machine that accumulates speech segments
// ─────────────────────────────────────────────────────────────────────────────

#include "core/types.h"
#include "core/config.h"
#include "vad/vad.h"

#include <vector>
#include <functional>

namespace localvoice {

/// State of the speech segmenter
enum class SegmenterState : uint8_t {
    Silence,     ///< No speech detected
    Speaking,    ///< Speech is ongoing, accumulating audio
    PostSpeech,  ///< Speech ended, waiting for min silence to confirm
};

/// Callback invoked when a complete speech segment is ready for inference
using SpeechSegmentCallback = std::function<void(std::vector<AudioSample>&& segment)>;

/// Manages the speech detection state machine and accumulates audio
/// for complete speech segments.
class SpeechSegmenter {
public:
    explicit SpeechSegmenter(const VadConfig& config);

    /// Process a chunk of 16 kHz mono audio through VAD and accumulate
    void process(AudioSpan samples, IVAD& vad, SpeechSegmentCallback on_segment);

    /// Force-finalize the current segment (e.g., push-to-talk release)
    void force_finalize(SpeechSegmentCallback on_segment);

    /// Reset state
    void reset();

    /// Get current state
    [[nodiscard]] SegmenterState state() const noexcept { return state_; }

    /// Get accumulated sample count
    [[nodiscard]] size_t accumulated_samples() const noexcept { return accumulator_.size(); }

private:
    void transition_to(SegmenterState new_state);
    void finalize_segment(SpeechSegmentCallback& on_segment);

    VadConfig             config_;
    SegmenterState        state_ = SegmenterState::Silence;

    // Pre-allocated speech accumulator buffer (max 30s @ 16kHz)
    std::vector<AudioSample> accumulator_;

    // Timing
    size_t silence_samples_ = 0;   ///< Consecutive silence samples
    size_t speech_samples_  = 0;   ///< Total speech samples in current segment

    // Derived values (calculated once from config)
    size_t min_speech_samples_  = 0;
    size_t min_silence_samples_ = 0;
    size_t max_speech_samples_  = 0;
    size_t speech_pad_samples_  = 0;
};

} // namespace localvoice
