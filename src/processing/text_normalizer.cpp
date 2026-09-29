#include "processing/text_normalizer.h"

#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>

namespace localvoice {

std::string TextNormalizer::normalize(const std::string& text) {
    if (text.empty()) return {};

    std::string result = text;

    // Remove common Whisper hallucination artifacts
    // These patterns appear when Whisper processes silence or noise
    static const std::vector<std::string> artifacts = {
        "[BLANK_AUDIO]",
        "(music)",
        "(Music)",
        "(MUSIC)",
        "(silence)",
        "(Silence)",
        "[Music]",
        "[MUSIC]",
        "(background noise)",
        "(Background noise)",
        "...",
        "(laughing)",
        "(applause)",
        "(coughing)",
        "( )",
        "(inaudible)",
    };

    for (const auto& artifact : artifacts) {
        size_t pos;
        while ((pos = result.find(artifact)) != std::string::npos) {
            result.erase(pos, artifact.size());
        }
    }

    // Trim leading and trailing whitespace
    auto start = result.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return {};
    auto end = result.find_last_not_of(" \t\n\r");
    result = result.substr(start, end - start + 1);

    // Collapse multiple spaces into single space
    std::string collapsed;
    collapsed.reserve(result.size());
    bool prev_space = false;
    for (char c : result) {
        if (c == ' ' || c == '\t') {
            if (!prev_space) {
                collapsed += ' ';
                prev_space = true;
            }
        } else {
            collapsed += c;
            prev_space = false;
        }
    }

    return collapsed;
}

} // namespace localvoice
