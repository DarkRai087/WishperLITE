#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// LocalVoice version information
// ─────────────────────────────────────────────────────────────────────────────

#define LOCALVOICE_VERSION_MAJOR 0
#define LOCALVOICE_VERSION_MINOR 1
#define LOCALVOICE_VERSION_PATCH 0
#define LOCALVOICE_VERSION_STRING "0.1.0"

namespace localvoice {

struct Version {
    static constexpr int major = LOCALVOICE_VERSION_MAJOR;
    static constexpr int minor = LOCALVOICE_VERSION_MINOR;
    static constexpr int patch = LOCALVOICE_VERSION_PATCH;
    static constexpr const char* string = LOCALVOICE_VERSION_STRING;
};

} // namespace localvoice
