#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// ITranscriptSink — interface for transcript output destinations
// ─────────────────────────────────────────────────────────────────────────────

#include "core/types.h"
#include <string>

namespace localvoice {

/// Abstract transcript output sink
class ITranscriptSink {
public:
    virtual ~ITranscriptSink() = default;

    /// Called when new transcribed text is available
    virtual void on_transcript(const std::string& text, const TranscriptResult& result) = 0;

    /// Called when transcription state changes
    virtual void on_state_changed(AppState state) = 0;

    /// Called on errors
    virtual void on_error(const std::string& message) = 0;
};

} // namespace localvoice
