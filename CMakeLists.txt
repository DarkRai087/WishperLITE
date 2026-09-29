cmake_minimum_required(VERSION 3.25)

# ─────────────────────────────────────────────────────────────────────────────
# LocalVoice — Lightweight Offline Speech-to-Text for Windows
# ─────────────────────────────────────────────────────────────────────────────
project(LocalVoice
    VERSION 0.1.0
    DESCRIPTION "Lightweight offline English speech-to-text desktop application"
    LANGUAGES CXX
)

# ─────────────────────────────────────────────────────────────────────────────
# C++20 Standard
# ─────────────────────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ─────────────────────────────────────────────────────────────────────────────
# Options
# ─────────────────────────────────────────────────────────────────────────────
option(LOCALVOICE_BUILD_TESTS      "Build unit and integration tests"  ON)
option(LOCALVOICE_BUILD_BENCHMARKS "Build performance benchmarks"      ON)
option(LOCALVOICE_ENABLE_CUDA      "Enable CUDA GPU acceleration"     OFF)

# ─────────────────────────────────────────────────────────────────────────────
# Compiler flags
# ─────────────────────────────────────────────────────────────────────────────
include(cmake/CompilerFlags.cmake)

# ─────────────────────────────────────────────────────────────────────────────
# Third-party dependencies
# ─────────────────────────────────────────────────────────────────────────────

# nlohmann/json (single-header, vendored)
add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE ${CMAKE_SOURCE_DIR}/third_party/json)

# whisper.cpp will be added as a subdirectory once the submodule is cloned
# For Phase 2, we build the core application skeleton without whisper.cpp
# The ISTTEngine / IVAD interfaces allow us to compile without it.

# ─────────────────────────────────────────────────────────────────────────────
# Core library (all non-UI, non-platform code)
# ─────────────────────────────────────────────────────────────────────────────
add_library(localvoice_core STATIC
    # Core module
    src/core/application.cpp
    src/core/config.cpp
    src/core/event_bus.cpp
    src/core/logger.cpp
    src/core/diagnostics.cpp

    # Audio module
    src/audio/wasapi_capture.cpp
    src/audio/resampler.cpp
    src/audio/audio_preprocessor.cpp

    # Processing module
    src/processing/text_normalizer.cpp
    src/processing/transcript_segmenter.cpp

    # Output module
    src/output/ui_transcript_sink.cpp
)

target_include_directories(localvoice_core PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(localvoice_core PUBLIC
    nlohmann_json
)

# Windows-specific link libraries for WASAPI, COM, hotkeys
if(WIN32)
    target_link_libraries(localvoice_core PUBLIC
        ole32          # COM
        oleaut32       # COM Automation
        uuid           # COM GUIDs
        avrt           # MMCSS (Multimedia Class Scheduler)
        winmm          # Multimedia timers
        ksuser         # Audio
    )
    target_compile_definitions(localvoice_core PUBLIC
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        UNICODE
        _UNICODE
    )
endif()

# ─────────────────────────────────────────────────────────────────────────────
# Platform library (Win32 UI, tray, hotkeys)
# ─────────────────────────────────────────────────────────────────────────────
add_library(localvoice_platform STATIC
    src/platform/tray_icon.cpp
    src/platform/hotkey_manager.cpp
    src/platform/main_window.cpp
)

target_include_directories(localvoice_platform PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(localvoice_platform PUBLIC
    localvoice_core
)

# ─────────────────────────────────────────────────────────────────────────────
# Main executable
# ─────────────────────────────────────────────────────────────────────────────
add_executable(LocalVoice WIN32
    src/main.cpp
)

target_link_libraries(LocalVoice PRIVATE
    localvoice_core
    localvoice_platform
)

# ─────────────────────────────────────────────────────────────────────────────
# Tests
# ─────────────────────────────────────────────────────────────────────────────
if(LOCALVOICE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ─────────────────────────────────────────────────────────────────────────────
# Benchmarks
# ─────────────────────────────────────────────────────────────────────────────
if(LOCALVOICE_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# ─────────────────────────────────────────────────────────────────────────────
# Install
# ─────────────────────────────────────────────────────────────────────────────
install(TARGETS LocalVoice RUNTIME DESTINATION bin)
install(DIRECTORY configs/ DESTINATION configs)
install(DIRECTORY models/ DESTINATION models FILES_MATCHING PATTERN "*.gitkeep")
