#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// Transcript segmenter — merge/format transcript segments for display
// ─────────────────────────────────────────────────────────────────────────────

#include "core/types.h"
#include <string>
#include <vector>

namespace localvoice {

class TranscriptSegmenter {
public:
    /// Add a new transcript result and return formatted display text
    std::string add_result(const TranscriptResult& result);

    /// Get the full transcript history
    [[nodiscard]] const std::vector<std::string>& history() const { return history_; }

    /// Clear history
    void clear();

private:
    std::vector<std::string> history_;
    static constexpr size_t kMaxHistory = 100;  ///< Keep last N segments
};

} // namespace localvoice
