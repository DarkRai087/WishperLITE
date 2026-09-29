#include "processing/transcript_segmenter.h"
#include "processing/text_normalizer.h"

namespace localvoice {

std::string TranscriptSegmenter::add_result(const TranscriptResult& result) {
    std::string combined;
    for (const auto& seg : result.segments) {
        std::string normalized = TextNormalizer::normalize(seg.text);
        if (!normalized.empty()) {
            if (!combined.empty()) combined += " ";
            combined += normalized;
        }
    }

    if (!combined.empty()) {
        history_.push_back(combined);

        // Trim old history to prevent unbounded growth
        while (history_.size() > kMaxHistory) {
            history_.erase(history_.begin());
        }
    }

    return combined;
}

void TranscriptSegmenter::clear() {
    history_.clear();
}

} // namespace localvoice
