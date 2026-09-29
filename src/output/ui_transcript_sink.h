#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// UITranscriptSink — outputs transcripts to the Win32 main window
// ─────────────────────────────────────────────────────────────────────────────

#include "output/transcript_sink.h"

namespace localvoice {

/// Forward declaration
class MainWindow;

class UITranscriptSink : public ITranscriptSink {
public:
    void on_transcript(const std::string& text, const TranscriptResult& result) override;
    void on_state_changed(AppState state) override;
    void on_error(const std::string& message) override;

    /// Set the target window (called during initialization)
    void set_window(MainWindow* window) { window_ = window; }

private:
    MainWindow* window_ = nullptr;
};

} // namespace localvoice
