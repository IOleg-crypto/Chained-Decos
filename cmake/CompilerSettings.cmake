# Chained Engine - Compiler Settings
# Extracted from root CMakeLists.txt for modularity

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_DEBUG_POSTFIX "")

# Standardize output directories for all targets (Set BEFORE including dependencies)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Generate compile_commands.json")
set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
if(UNIX AND NOT APPLE)
    set(CMAKE_INSTALL_RPATH "$ORIGIN")
endif()

set(CMAKE_DEBUG_POSTFIX "")

# Compiler-specific settings
if(MSVC AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # MSVC (Standardcl.exe)
    include(${CMAKE_CURRENT_LIST_DIR}/compilers/CompilerMSVC.cmake)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Clang settings (includes AppleClang and clang-cl)
    include(${CMAKE_CURRENT_LIST_DIR}/compilers/CompilerClang.cmake)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # GCC settings
    include(${CMAKE_CURRENT_LIST_DIR}/compilers/CompilerGCC.cmake)
endif()

# Platform-specific settings
if(WIN32)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    add_compile_definitions(WIN32_LEAN_AND_MEAN)
    add_compile_definitions(NOMINMAX)
    add_compile_definitions(_WIN32_WINNT=0x0601)  # Windows 7+
endif()

# Optimized Build Settings
option(ENABLE_UNITY_BUILD "Enable Unity Builds for faster compilation" OFF)
option(ENABLE_PCH "Enable Precompiled Headers for faster compilation" ON)
option(ENABLE_LTO "Enable Link-Time Optimization (IPO) for Release configurations" ON)
option(ENABLE_COVERAGE "Enable Code Coverage (GCC/Clang only)" OFF)
# Warning Settings
option(DISABLE_ALL_WARNINGS "Disable all compiler warnings" OFF)
option(ENABLE_WARNINGS "Enable compiler warnings" OFF)
option(WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)


if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(--coverage -fprofile-arcs -ftest-coverage)
        add_link_options(--coverage)
    endif()
endif()

if(ENABLE_UNITY_BUILD)
    set(CMAKE_UNITY_BUILD ON)
    set(CMAKE_UNITY_BUILD_BATCH_SIZE 16)
endif()

# ── Engine-wide PCH ──────────────────────────────────────────────────────────
# INTERFACE library that propagates the engine precompiled header to any target
# that links it privately.  Each consumer creates its own .pch binary matching
# its own compiler flags, so there is no cross-TU contamination.
if(ENABLE_PCH)
    add_library(engine_pch INTERFACE)
    target_precompile_headers(engine_pch INTERFACE "${PROJECT_SOURCE_DIR}/engine/engine_pch.h")
else()
    # Force-include: injects engine_pch.h into every TU via compiler flags.
    # Gives the same include coverage as PCH but without a .pch binary,
    # so ccache/sccache still achieves 100% hit rates in CI.
    add_library(engine_pch INTERFACE)
    set(_pch_path "${PROJECT_SOURCE_DIR}/engine/engine_pch.h")
    if(MSVC)
        target_compile_options(engine_pch INTERFACE "/FI${_pch_path}")
    else()
        target_compile_options(engine_pch INTERFACE "-include" "${_pch_path}")
    endif()
endif()

# ── LTO (global) ────────────────────────────────────────────────────────────
# Applied once here so individual targets don't need to set it.
if(ENABLE_LTO AND NOT MINGW)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
endif()
