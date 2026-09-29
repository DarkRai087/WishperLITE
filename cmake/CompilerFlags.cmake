# ─────────────────────────────────────────────────────────────────────────────
# Compiler flags for MSVC and compatible compilers
# ─────────────────────────────────────────────────────────────────────────────

if(MSVC)
    # Warning level 4 + treat warnings as errors in CI
    add_compile_options(/W4)

    # Conformance mode
    add_compile_options(/permissive-)

    # UTF-8 source and execution character set
    add_compile_options(/utf-8)

    # Enable parallel compilation
    add_compile_options(/MP)

    # Disable specific noisy warnings that don't indicate real problems
    # C4100: unreferenced formal parameter (common in interface implementations)
    add_compile_options(/wd4100)

    # Release optimizations
    if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        add_compile_options(/O2)    # Maximize speed
        add_compile_options(/Oi)    # Enable intrinsic functions
        add_compile_options(/GL)    # Whole program optimization
        add_link_options(/LTCG)     # Link-time code generation
    endif()

    # Debug: enable runtime checks
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(/RTC1)  # Runtime error checks
        add_compile_options(/sdl)   # Security development lifecycle checks
    endif()

elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Fallback for MinGW/Clang development
    add_compile_options(-Wall -Wextra -Wpedantic)
    add_compile_options(-Wno-unused-parameter)

    if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        add_compile_options(-O2)
    endif()
endif()
