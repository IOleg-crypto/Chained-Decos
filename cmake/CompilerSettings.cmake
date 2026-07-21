# Chained Engine - Compiler Settings
# Extracted from root CMakeLists.txt for modularity

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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

# Function to apply common engine optimizations to a target
function(apply_engine_optimizations target_name)
    set(NO_PCH OFF)
    if(ARGC GREATER 1)
        if("${ARGV1}" STREQUAL "NO_PCH")
            set(NO_PCH ON)
        endif()
    endif()

    if(ENABLE_PCH AND NOT NO_PCH)
        # Real PCH: faster local dev, but breaks sccache cache hit rates.
        # PRIVATE: consumers that want the PCH call apply_engine_optimizations()
        # themselves instead of inheriting it transitively.
        target_precompile_headers(${target_name} PRIVATE "${PROJECT_SOURCE_DIR}/engine/engine_pch.h")
    elseif(NOT NO_PCH)
        # Force-include: injects engine_pch.h into every TU via compiler flags.
        # This gives the same include coverage as PCH but without a .pch binary,
        # so ccache/sccache still achieves 100% hit rates in CI.
        set(_pch_path "${PROJECT_SOURCE_DIR}/engine/engine_pch.h")
        if(MSVC)
            target_compile_options(${target_name} PRIVATE "/FI${_pch_path}")
        else()
            target_compile_options(${target_name} PRIVATE "-include" "${_pch_path}")
        endif()
    endif()

    if(ENABLE_LTO)
        # Disable LTO for MinGW/GCC in Debug as it's extremely slow
        if(MINGW OR (CMAKE_BUILD_TYPE STREQUAL "Debug"))
            set(ipo_supported OFF)
        else()
            include(CheckIPOSupported)
            check_ipo_supported(RESULT ipo_supported OUTPUT ipo_output)
        endif()

        if(ipo_supported)
            set_property(TARGET ${target_name} PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
        elseif(ipo_output AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
            message(STATUS "IPO/LTO is not supported or disabled for this configuration: ${ipo_output}")
        endif()
    endif()

    # Enable Unity Build for the target if global option is ON
    if(ENABLE_UNITY_BUILD)
        set_target_properties(${target_name} PROPERTIES UNITY_BUILD ON)
    endif()
endfunction()
