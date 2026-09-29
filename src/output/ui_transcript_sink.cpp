#include "output/ui_transcript_sink.h"
#include "platform/main_window.h"
#include "core/logger.h"

namespace localvoice {

void UITranscriptSink::on_transcript(const std::string& text, const TranscriptResult& result) {
    if (window_) {
        window_->append_text(text);
    }
    LV_DEBUG("output", "Transcript: " + text);
}

void UITranscriptSink::on_state_changed(AppState state) {
    if (window_) {
        window_->set_status(state);
    }
}

void UITranscriptSink::on_error(const std::string& message) {
    LV_ERROR("output", "Error: " + message);
}

} // namespace localvoice
