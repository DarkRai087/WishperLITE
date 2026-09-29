#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// Text normalizer — clean up raw whisper output
// ─────────────────────────────────────────────────────────────────────────────

#include <string>

namespace localvoice {

class TextNormalizer {
public:
    /// Normalize transcribed text:
    /// - Trim leading/trailing whitespace
    /// - Collapse multiple spaces
    /// - Remove common Whisper artifacts (e.g., [BLANK_AUDIO], (music))
    /// - Fix capitalization after sentence boundaries
    [[nodiscard]] static std::string normalize(const std::string& text);
};

} // namespace localvoice
